#ifndef __STREAM_GENERATOR__
#define __STREAM_GENERATOR__

#include <vector>
#include <deque>
#include <string>

#include "utils.hpp"
#include "ocl.hpp"


namespace fx {

#define MEMORY_MAP 0
#if MEMORY_MAP

template <typename T>
struct StreamGeneratorExecution
{
    OCL & ocl;
    size_t max_batch_size;
    size_t replica_id;

    cl_kernel kernel;
    cl_command_queue queue;

    cl_mem batch_d;
    T * batch_h;

    cl_event map_event;
    cl_event unmap_event;
    cl_event kernel_event;

    const int batch_argi = 0;
    const int count_argi = 1;
    const int eos_argi = 2;

    StreamGeneratorExecution(
        OCL & ocl,
        const size_t batch_size,
        const size_t replica_id = 0
    )
    : ocl(ocl)
    , max_batch_size(batch_size)
    , replica_id(replica_id)
    {
        cl_int err;

        kernel = ocl.createKernel("memory_reader:{memory_reader_" + std::to_string(replica_id + 1) + "}");
        queue = ocl.createCommandQueue();

        batch_h = aligned_alloc<T>(max_batch_size);
        batch_d = clCreateBuffer(
            ocl.context,
            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_WRITE_ONLY,
            max_batch_size * sizeof(T), nullptr,
            &err
        );
        clCheckErrorMsg(err, "fx::StreamGenerator: failed to create device buffer (batch_d)");

        clCheckError(clSetKernelArg(kernel, batch_argi, sizeof(batch_d), &batch_d));
    }

    void execute(
        size_t batch_size,
        bool eos
    )
    {
        if (batch_size > max_batch_size) {
            std::cerr
                << "fx::StreamGenerator: batch_size is larger than max_batch_size!"
                << "Only " << max_batch_size << " elements are processed."
                << '\n';

            batch_size = max_batch_size;
        }

        // TODO: (512 / 8) should be calculated at runtime based on the width of the bus selected for the memory reader
        const cl_int count_int = static_cast<cl_int>(batch_size / ((512 / 8) / sizeof(T)));
        const cl_int eos_int = static_cast<cl_int>(eos);

        clCheckError(clSetKernelArg(kernel, count_argi, sizeof(count_int), &count_int));
        clCheckError(clSetKernelArg(kernel, eos_argi,   sizeof(eos_int),   &eos_int));

        clCheckError(clEnqueueUnmapMemObject(queue, batch_d, batch_h, 0, nullptr, &unmap_event));
        batch_h = nullptr;

        clCheckError(clEnqueueTask(queue, kernel, 1, &unmap_event, &kernel_event));
    }

    T * get_batch_ptr()
    {
        return batch_h;
    }

    T * get_batch()
    {
        cl_int err;
        batch_h = static_cast<T *>(clEnqueueMapBuffer(
            queue, batch_d, CL_FALSE, CL_MAP_WRITE,
            0, max_batch_size * sizeof(T),
            0, nullptr, &map_event,
            &err
        ));
        clCheckErrorMsg(err, "fx::StreamGenerator: failed to map device buffer (batch_d)");

        clCheckError(clWaitForEvents(1, &map_event));
        clCheckError(clReleaseEvent(map_event));
        return batch_h;
    }

    void wait()
    {
        clCheckError(clWaitForEvents(1, &kernel_event));
        clCheckError(clReleaseEvent(kernel_event));
        clCheckError(clReleaseEvent(unmap_event));
    }

    ~StreamGeneratorExecution()
    {
        clCheckError(clReleaseMemObject(batch_d));
        clCheckError(clReleaseKernel(kernel));
        clCheckError(clReleaseCommandQueue(queue));

        // free(batch_h);
    }
};
#else

template <typename T>
struct StreamGeneratorExecution
{
    OCL & ocl;
    size_t max_batch_size;
    size_t replica_id;

    cl_kernel kernel;
    cl_command_queue queue;

    cl_mem batch_d;
    T * batch_h;

    cl_event migrate_event;
    cl_event kernel_event;

    const int batch_argi = 0;
    const int count_argi = 1;
    const int eos_argi = 2;

    StreamGeneratorExecution(
        OCL & ocl,
        const size_t batch_size,
        const size_t replica_id = 0
    )
    : ocl(ocl)
    , max_batch_size(batch_size)
    , replica_id(replica_id)
    {
        cl_int err;

        kernel = ocl.createKernel("memory_reader:{memory_reader_" + std::to_string(replica_id) + "}");
        queue = ocl.createCommandQueue();

        batch_h = aligned_alloc<T>(max_batch_size);
        batch_d = clCreateBuffer(
            ocl.context,
            CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_WRITE_ONLY,
            max_batch_size * sizeof(T), batch_h,
            &err
        );
        clCheckErrorMsg(err, "fx::StreamGenerator: failed to create device buffer (batch_d)");

        clCheckError(clSetKernelArg(kernel, batch_argi, sizeof(batch_d), &batch_d));
        clCheckError(clEnqueueMigrateMemObjects(
            queue, 1, &batch_d,
            CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
            0, nullptr, &migrate_event
        ));

        clCheckError(clWaitForEvents(1, &migrate_event));
        clCheckError(clReleaseEvent(migrate_event));
    }

    void execute(
        size_t batch_size,
        bool eos
    )
    {
        if (batch_size > max_batch_size) {
            std::cerr
                << "fx::StreamGenerator: batch_size is larger than max_batch_size!"
                << "Only " << max_batch_size << " elements are processed."
                << '\n';

            batch_size = max_batch_size;
        }

        // TODO: (512 / 8) should be calculated at runtime based on the width of the bus selected for the memory reader
        const cl_int count_int = static_cast<cl_int>(batch_size / ((512 / 8) / sizeof(T)));
        const cl_int eos_int = static_cast<cl_int>(eos);

        clCheckError(clSetKernelArg(kernel, count_argi, sizeof(count_int), &count_int));
        clCheckError(clSetKernelArg(kernel, eos_argi,   sizeof(eos_int),   &eos_int));

        clCheckError(clEnqueueMigrateMemObjects(
            queue,
            1, &batch_d, 0,
            0, nullptr, &migrate_event
        ));

        clCheckError(clEnqueueTask(queue, kernel, 1, &migrate_event, &kernel_event));
    }

    T * get_batch_ptr() // used to be conformant with the mapped version
    {
        return batch_h;
    }

    T * get_batch()
    {
        return batch_h;
    }

    void wait()
    {
        clCheckError(clWaitForEvents(1, &kernel_event));
        clCheckError(clReleaseEvent(kernel_event));
        clCheckError(clReleaseEvent(migrate_event));
    }

    ~StreamGeneratorExecution()
    {
        clCheckError(clReleaseMemObject(batch_d));
        clCheckError(clReleaseKernel(kernel));
        clCheckError(clReleaseCommandQueue(queue));

        free(batch_h);
    }
};
#endif

template <typename T>
struct StreamGenerator
{
    using ExecutionQueue = std::deque<StreamGeneratorExecution<T> *>;

    OCL & ocl;

    size_t max_batch_size;
    size_t number_of_buffers;
    size_t replica_id;
    size_t iterations;

    ExecutionQueue ready_queue;
    ExecutionQueue running_queue;

    T * current_batch;
    size_t batch_idx;

    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;

    StreamGenerator(
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
    , ready_queue()
    , running_queue()
    , current_batch(nullptr)
    , batch_idx(0)
    {
        if (batch_size != max_batch_size) {
            std::cout << "fx::StreamGenerator: `batch_size` is rounded to the next power of 2 ("
                      << batch_size << " -> " << max_batch_size << ")" << '\n';
        }

        for (size_t n = 0; n < number_of_buffers; ++n) {
            running_queue.push_back(new StreamGeneratorExecution<T>(ocl, max_batch_size, replica_id));
        }
    }

    T * get_batch()
    {
        if (iterations == 0) {
            start_time = std::chrono::high_resolution_clock::now();
        }

        StreamGeneratorExecution<T> * execution = running_queue.front();
        running_queue.pop_front();

        if (iterations >= number_of_buffers) {
            execution->wait();
        }

        T * batch = execution->get_batch();
        ready_queue.push_back(execution);

        return batch;
    }

    void push(
        T * batch,
        const size_t batch_size,
        const bool last = false
    )
    {
        StreamGeneratorExecution<T> * execution = ready_queue.front();
        ready_queue.pop_front();

        if (batch != execution->get_batch_ptr()) {
            std::cerr << "fx::StreamGenerator: batch pointer mismatch!" << '\n';
        }

        execution->execute(batch_size, last);
        running_queue.push_back(execution);

        iterations++;
    }

    void push(const T & item, const bool last = false)
    {
        if (current_batch == nullptr) {
            current_batch = get_batch();
            batch_idx = 0;
        }

        current_batch[batch_idx++] = item;

        if (last or (batch_idx == max_batch_size)) {
            push(current_batch, batch_idx, last);
            current_batch = nullptr;
        }
    }

    void launch_kernels() {}

    void finish()
    {
        while (!running_queue.empty()) {
            StreamGeneratorExecution<T> * execution = running_queue.front();
            running_queue.pop_front();
            execution->wait();
            ready_queue.push_back(execution);
        }

        end_time = std::chrono::high_resolution_clock::now();
    }

    auto get_start_time() { return start_time; }
    auto get_end_time() { return end_time; }
    double get_time() { return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count(); }

    ~StreamGenerator()
    {
        finish();

        while (!ready_queue.empty()) {
            StreamGeneratorExecution<T> * execution = ready_queue.front();
            ready_queue.pop_front();
            delete execution;
        }
    }
};

} // namespace fx

#endif // __STREAM_GENERATOR__
