#ifndef __STREAM_DRAINER__
#define __STREAM_DRAINER__

#include <vector>
#include <deque>
#include <string>

#include "defines.hpp"
#include "utils.hpp"
#include "ocl.hpp"


namespace fx {

template <typename T>
struct StreamDrainerExecution {

    OCL & ocl;
    size_t max_batch_size;
    const cl_mem * eos_d;
    size_t replica_id;

    cl_kernel kernel;
    cl_command_queue queue;

    cl_mem batch_d;
    cl_mem items_written_d;

    T * batch_h;
    cl_int * items_written_h;

    cl_event kernel_event;
    cl_event migrate_event;

    const int batch_argi = 1;
    const int bs_argi = 2;
    const int count_argi = 3;
    const int eos_argi = 4;


    StreamDrainerExecution(
        OCL & ocl,
        size_t batch_size,
        const cl_mem * eos_d,
        size_t replica_id = 0
    )
    : ocl(ocl)
    , max_batch_size(batch_size)
    , eos_d(eos_d)
    , replica_id(replica_id)
    {
        cl_int err;

        kernel = ocl.createKernel("memory_writer:{memory_writer_" + std::to_string(replica_id) + "}");
        queue = ocl.createCommandQueue();

        batch_h = aligned_alloc<T>(batch_size);

        batch_d = clCreateBuffer(
            ocl.context,
            CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY | CL_MEM_WRITE_ONLY,
            batch_size * sizeof(T), batch_h,
            &err
        );
        clCheckErrorMsg(err, "fx::StreamDrainer: failed to create device buffer (batch_d)");

        items_written_h = aligned_alloc<cl_int>(1);
        items_written_h[0] = 0;

        items_written_d = clCreateBuffer(
            ocl.context,
            CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY | CL_MEM_WRITE_ONLY,
            sizeof(cl_int), items_written_h,
            &err
        );
        clCheckErrorMsg(err, "fx::StreamDrainer: failed to create device buffer (items_written_d)");

        clCheckError(clSetKernelArg(kernel, batch_argi, sizeof(batch_d), &batch_d));
        clCheckError(clSetKernelArg(kernel, count_argi, sizeof(items_written_d), &items_written_d));
        clCheckError(clSetKernelArg(kernel, eos_argi,   sizeof(*eos_d),  eos_d));

        cl_mem buffers[] = {batch_d, items_written_d};
        clCheckError(clEnqueueMigrateMemObjects(
            queue, 2, buffers,
            CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
            0, nullptr, &migrate_event
        ));

        clCheckError(clWaitForEvents(1, &migrate_event));
        clCheckError(clReleaseEvent(migrate_event));
    }

    void execute(size_t batch_size)
    {
        if (batch_size > max_batch_size) {
            std::cerr
                << "fx::StreamDrainer: batch_size is larger than max_batch_size!"
                << "Only " << max_batch_size << " elements are processed."
                << '\n';

            batch_size = max_batch_size;
        }

        // TODO: cl_int should be cl_ulong to match the size of "size_t"
        const cl_int _bs = static_cast<cl_int>(batch_size * sizeof(T));
        clCheckError(clSetKernelArg(kernel, bs_argi, sizeof(_bs), &_bs));

        clCheckError(clEnqueueTask(queue, kernel, 0, nullptr, &kernel_event));

        cl_mem buffers[] = {batch_d, items_written_d, *eos_d};
        clCheckError(clEnqueueMigrateMemObjects(
            queue, 3, buffers,
            CL_MIGRATE_MEM_OBJECT_HOST,
            1, &kernel_event, &migrate_event
        ));
    }

    void wait()
    {
        clCheckError(clWaitForEvents(1, &migrate_event));

        #if STREAM_DRAINER_PRINT_TRANSFER_INFO
        {
            cl_ulong start_time;
            cl_ulong end_time;
            clCheckError(clGetEventProfilingInfo(migrate_event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL));
            clCheckError(clGetEventProfilingInfo(migrate_event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL));
            const double time = end_time - start_time;
            const double size = max_batch_size * sizeof(T);
            const double bw = size / time;
            std::cout << "fx::StreamDrainer (MIGRATE): " << bw << " GB/s" << "(start: " << start_time << ", end: " << end_time << ")" << '\n';
        }
        {
            cl_ulong start_time;
            cl_ulong end_time;
            clCheckError(clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL));
            clCheckError(clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL));
            const double time = end_time - start_time;
            const double size = max_batch_size * sizeof(T);
            const double bw = size / time;
            std::cout << "fx::StreamDrainer (KERNEL): " << bw << " GB/s" << "(start: " << start_time << ", end: " << end_time << ")" << '\n';
        }
        #endif

        clCheckError(clReleaseEvent(migrate_event));
        clCheckError(clReleaseEvent(kernel_event));
    }

    ~StreamDrainerExecution()
    {
        clCheckError(clFinish(queue));

        clCheckError(clReleaseMemObject(batch_d));
        clCheckError(clReleaseMemObject(items_written_d));
        clCheckError(clReleaseKernel(kernel));
        clCheckError(clReleaseCommandQueue(queue));

        free(items_written_h);
        free(batch_h);
    }
};

template <typename T>
struct StreamDrainer
{
    using ExecutionQueue = std::deque<StreamDrainerExecution<T> *>;

    OCL & ocl;

    size_t max_batch_size;
    size_t number_of_buffers;
    size_t replica_id;
    size_t iterations;

    cl_mem eos_d;
    cl_int * eos;

    ExecutionQueue ready_queue;
    ExecutionQueue running_queue;

    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;

    StreamDrainer(
        OCL & ocl,
        const size_t batch_size,
        const size_t N = 2,
        const size_t replica_id = 0
    )
    : ocl(ocl)
    , max_batch_size(next_pow2(batch_size))
    , number_of_buffers(N)
    , replica_id(replica_id)
    , iterations(0)
    , eos_d(nullptr)
    , eos(nullptr)
    , ready_queue()
    , running_queue()
    {
        if (batch_size != max_batch_size) {
            std::cout << "fx::StreamDrainer: `batch_size` is rounded to the next power of 2 ("
                      << batch_size << " -> " << max_batch_size << ")" << '\n';
        }

        cl_int err;

        eos = aligned_alloc<cl_int>(1);
        eos[0] = 0;

        cl_mem_ext_ptr_t eos_ext;
        eos_ext.obj = eos;
        eos_ext.param = 0;
        eos_ext.flags = hbm_bank[31];

        eos_d = clCreateBuffer(
            ocl.context,
            CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
            sizeof(cl_int), &eos_ext,
            &err
        );
        clCheckErrorMsg(err, "fx::StreamDrainer: failed to create device buffer (eos_d)");

        cl_command_queue queue = ocl.createCommandQueue();
        clCheckError(clEnqueueMigrateMemObjects(
            queue, 1, &eos_d,
            0,
            0, nullptr, nullptr
        ));

        clCheckError(clFinish(queue));
        clCheckError(clReleaseCommandQueue(queue));

        for (size_t n = 0; n < number_of_buffers; ++n) {
            ready_queue.push_back(new StreamDrainerExecution<T>(ocl, max_batch_size, &eos_d, replica_id));
        }
    }

    void prelaunch()
    {
        for (size_t n = 0; n < number_of_buffers; ++n) {
            launch_kernel(max_batch_size);
        }
    }

    void launch_kernel(size_t batch_size)
    {
        StreamDrainerExecution<T> * execution = ready_queue.front();
        ready_queue.pop_front();
        execution->execute(batch_size);
        running_queue.push_back(execution);
    }

    T * pop(
        size_t * items_written,
        bool * last)
    {
        if (running_queue.empty()) {
            std::cerr << "fx::StreamDrainer: no launched executions" << '\n';
            return nullptr;
        }

        StreamDrainerExecution<T> * execution = running_queue.front();
        running_queue.pop_front();
        execution->wait();

        T * batch = execution->batch_h;
        *items_written = execution->items_written_h[0];
        *last = (eos[0] ? true : false);

        ready_queue.push_back(execution);

        iterations++;

        return batch;
    }

    void put_batch(T * batch, size_t batch_size)
    {
        if (!batch) {
            std::cerr << "fx::StreamDrainer: batch is nullptr" << '\n';
            return;
        }

        launch_kernel(batch_size);
    }

    void launch_kernels() {}

    void finish()
    {
        while (!running_queue.empty()) {
            StreamDrainerExecution<T> * execution = running_queue.front();
            running_queue.pop_front();
            execution->wait();
            ready_queue.push_back(execution);
        }
    }

    ~StreamDrainer()
    {
        finish();

        while (!ready_queue.empty()) {
            StreamDrainerExecution<T> * execution = ready_queue.front();
            ready_queue.pop_front();
            delete execution;
        }

        clReleaseMemObject(eos_d);
        free(eos);
    }
};

} // namespace fx

#endif // __STREAM_DRAINER__
