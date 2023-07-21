#include "fx_opencl.hpp"

#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>
#include <array>
#include <deque>
#include <thread>
#include <string>
#include <pthread.h>
#include <cassert>
#include <string.h> // memcpy

#include "../common/tuple.hpp"
#include "sd_dataset.hpp"

size_t _n_sink = 0;

pthread_barrier_t barrier;

template <typename T>
void print_time(
    std::string name,
    size_t number_of_tuples,
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end,
    bool is_bandwidth = true)
{
    auto timeDiff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    if (is_bandwidth) {
        double thr = number_of_tuples * sizeof(T);
        thr *= 1000000;     // convert us to s
        thr /= 1024 * 1024; // convert to MB
        thr /= timeDiff;

        std::cout
            << COUT_HEADER << name
            << COUT_HEADER << "\tTime: "       << COUT_FLOAT << timeDiff / 1000 << " ms"
            << COUT_HEADER << "\tThroughput: " << COUT_FLOAT << thr             << " MB/s"
            << '\n';
    } else {
        double thr = number_of_tuples;
        thr /= timeDiff;
        thr *= 1000000;   // convert us to s
        size_t tuple_sec = thr / 1000; // convert to Kilo tuples/sec
        std::cout
            << COUT_HEADER << name
            << COUT_HEADER << "\tTime: "       << COUT_FLOAT   << timeDiff / 1000  << " ms"
            << COUT_HEADER << "\tKTuple/sec: " << COUT_INTEGER << tuple_sec
            << '\n';
    }
}

auto get_time()
{
    return std::chrono::high_resolution_clock::now();
}

record_t new_record(int i)
{
    record_t r;
    r.key = static_cast<unsigned int>(i % record_t::MAX_KEY_VALUE);
    r.property_value = static_cast<float>(i);
    r.incremental_average = 0;
    r.timestamp = 0;
    return r;
}

void print_record(const record_t & r)
{
    std::cout << "("
    << r.key << ", "
    << r.property_value << ", "
    << r.incremental_average << ", "
    << r.timestamp
    << ")"
    << '\n';
}

void print_batch(record_t * batch, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        print_record(batch[i]);
    }
}

template <typename T>
void fill_batch_with_dataset(const std::vector<T> & dataset, T * batch, size_t size)
{
    // size_t t_index = 0;
    // for (int i = 0; i < size; ++i) {
    //     batch[i] = dataset[t_index];
    //     t_index = (t_index + 1) % dataset.size();
    // }

    size_t n = size / dataset.size();
    size_t remaining_elems = size % dataset.size();

    T * bufferStart = batch;
    for (size_t i = 0; i < n; i++) {
        // std::copy(dataset.begin(), dataset.end(), bufferStart);

        // use memcpy
        memcpy(bufferStart, dataset.data(), dataset.size() * sizeof(T));

        bufferStart += dataset.size();
    }

    if (remaining_elems > 0) {
        // std::copy(dataset.begin(), dataset.begin() + remaining_elems, bufferStart);
        // use memcpy
        memcpy(bufferStart, dataset.data(), remaining_elems * sizeof(T));
    }
}

namespace fx {

#define MAX_HBM_PC_COUNT 32
#define PC_NAME(n) (n | XCL_MEM_TOPOLOGY)
const int pc_[MAX_HBM_PC_COUNT] = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15),
    PC_NAME(16), PC_NAME(17), PC_NAME(18), PC_NAME(19), PC_NAME(20), PC_NAME(21), PC_NAME(22), PC_NAME(23),
    PC_NAME(24), PC_NAME(25), PC_NAME(26), PC_NAME(27), PC_NAME(28), PC_NAME(29), PC_NAME(30), PC_NAME(31)};


template <typename T>
struct Source_Execution
{
    OCL & ocl;

    size_t batch_size;

    cl_mem batch_d;
    T * batch_h;

    cl_command_queue queue;
    cl_kernel kernel;
    cl_event migrate_event;
    cl_event kernel_event;

    int i;

    Source_Execution(
        OCL & ocl,
        cl_command_queue q,
        const size_t batch_size,
        int i = -1)
    : ocl(ocl)
    // , queue(queue)
    , batch_size(batch_size)
    , batch_d(nullptr)
    , batch_h(nullptr)
    , i(i)
    {
        cl_int err;

        queue = ocl.createCommandQueue(true);
        kernel = ocl.createKernel("memory_reader");

        batch_h = aligned_alloc<T>(batch_size);

        cl_mem_ext_ptr_t batch_ext;
        batch_ext.obj = batch_h;
        batch_ext.param = 0;
        batch_ext.flags = pc_[0];

        batch_d = clCreateBuffer(
            ocl.context,
            CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
            batch_size * sizeof(T), &batch_ext,
            &err
        );
        clCheckErrorMsg(err, "Failed to create batch_d");
    }

    void execute(bool eos)
    {
        // std::cout << "Executing SourceExecution: " << i << '\n';
        const cl_int count_int = static_cast<cl_int>(batch_size / ((512 / 8) / sizeof(T)));
        const cl_int eos_int = static_cast<cl_int>(eos);

        cl_uint argi = 0;
        clCheckError(clSetKernelArg(kernel, argi++, sizeof(batch_d),   &batch_d));
        clCheckError(clSetKernelArg(kernel, argi++, sizeof(count_int), &count_int));
        clCheckError(clSetKernelArg(kernel, argi++, sizeof(eos_int),   &eos_int));

        clCheckError(clEnqueueMigrateMemObjects(
            queue,
            1, &batch_d, 0,
            0, nullptr, &migrate_event
        ));
        // clFlush(queue);

        clCheckError(clEnqueueTask(queue, kernel, 1, &migrate_event, &kernel_event));
        // clFlush(queue);
    }

    T * get_batch()
    {
        return batch_h;
    }

    void wait()
    {
        // std::cout << "Waiting SourceExecution: " << i << '\n';
        clWaitForEvents(1, &kernel_event);
        clCheckError(clReleaseEvent(kernel_event));
        clCheckError(clReleaseEvent(migrate_event));
    }

    ~Source_Execution()
    {
        clReleaseMemObject(batch_d);
        clReleaseKernel(kernel);

        free(batch_h);
    }
};

template <typename T>
struct Source
{
    OCL & ocl;

    size_t max_batch_size;
    size_t number_of_buffers;
    size_t iterations;

    cl_command_queue queue;
    std::deque<Source_Execution<T> *> waiting_executions;
    std::deque<Source_Execution<T> *> running_executions;

    Source(
        OCL & ocl,
        const size_t batch_size,
        const size_t N)
    : ocl(ocl)
    , max_batch_size(next_pow2(batch_size))
    , number_of_buffers(N)
    , iterations(0)
    , waiting_executions()
    , running_executions()
    {
        if (batch_size != max_batch_size) {
            std::cout << "fx::Source: `batch_size` is rounded to the next power of 2 ("
                      << batch_size << " -> " << max_batch_size << ")" << '\n';
        }

        queue = ocl.createCommandQueue();

        for (size_t n = 0; n < number_of_buffers; ++n) {
            running_executions.push_back(new Source_Execution<T>(ocl, queue, max_batch_size, n));
        }
    }

    T * get_batch()
    {
        Source_Execution<T> * execution = running_executions.front();
        running_executions.pop_front();

        if (iterations >= number_of_buffers) {
            execution->wait();
        }

        T * batch = execution->get_batch();
        waiting_executions.push_back(execution);

        return batch;
    }

    void push(
        T * batch,
        const size_t batch_elems,
        const bool last = false)
    {
        Source_Execution<T> * execution = waiting_executions.front();
        waiting_executions.pop_front();
        execution->execute(last);
        running_executions.push_back(execution);

        iterations++;
    }

    void launch_kernels() {}

    void finish()
    {
        while (!running_executions.empty()) {
            Source_Execution<T> * execution = running_executions.front();
            running_executions.pop_front();
            execution->wait();
            waiting_executions.push_back(execution);
        }

        clFinish(queue);
    }

    ~Source()
    {
        finish();

        while (!waiting_executions.empty()) {
            Source_Execution<T> * execution = waiting_executions.front();
            waiting_executions.pop_front();
            delete execution;
        }

        while (!running_executions.empty()) {
            Source_Execution<T> * execution = running_executions.front();
            running_executions.pop_front();
            delete execution;
        }

        clCheckError(clReleaseCommandQueue(queue));
    }

};

template <typename T>
struct SourceTask
{
    OCL & ocl;
    size_t max_batch_size;
    cl_command_queue queue;

    size_t iterations;

    SourceTask(
        OCL & ocl,
        const size_t batch_size,
        const size_t N)
    : ocl(ocl)
    , max_batch_size(next_pow2(batch_size))
    , iterations(0)
    {
        (void)N;
        if (batch_size != max_batch_size) {
            std::cout << "fx::Source: `batch_size` is rounded to the next power of 2 ("
                      << batch_size << " -> " << max_batch_size << ")" << '\n';
        }

        queue = ocl.createCommandQueue(true);
    }

    Source_Execution<T> * get_task()
    {
        Source_Execution<T> * execution = new Source_Execution<T>(ocl, queue, max_batch_size, iterations);
        return execution;
    }

    void push(
        Source_Execution<T> * execution,
        const bool last = false)
    {
        execution->execute(last);
        iterations++;
    }

    void launch_kernels() {}

    void finish()
    {
        clFinish(queue);
    }

    ~SourceTask()
    {
        finish();

        clCheckError(clReleaseCommandQueue(queue));
    }

};

template <typename T>
    struct Sink_Execution {

        OCL & ocl;
        cl_command_queue queue;
        cl_mem batch_d;
        cl_mem count_d;
        // TODO: verificare che const non dia problemi
        const cl_mem * eos_d;       // contains a reference of a global eos

        T * batch_h;
        cl_int * count_h;

        cl_kernel kernel;
        cl_event kernel_event;
        cl_event migrate_event;

        Sink_Execution(
            OCL & ocl,
            cl_command_queue queue,
            size_t batch_size,
            const cl_mem * eos_d)
        : ocl(ocl)
        , queue(queue)
        , batch_d(nullptr)
        , count_d(nullptr)
        , eos_d(eos_d)
        , batch_h(nullptr)
        , count_h(0)
        {
            cl_int err;

            kernel = ocl.createKernel("memory_writer");

            batch_h = aligned_alloc<T>(batch_size);

            cl_mem_ext_ptr_t batch_ext;
            batch_ext.obj = batch_h;
            batch_ext.param = 0;
            batch_ext.flags = pc_[30];

            // TODO: usare MAP/UNMAP puo' essere meglio?
            batch_d = clCreateBuffer(
                ocl.context,
                CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                batch_size * sizeof(T), &batch_ext,
                &err
            );
            clCheckErrorMsg(err, "Failed to create batch_d");

            count_h = aligned_alloc<cl_int>(1);

            cl_mem_ext_ptr_t count_ext;
            count_ext.obj = count_h;
            count_ext.param = 0;
            count_ext.flags = pc_[31];

            count_d = clCreateBuffer(
                ocl.context,
                CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                sizeof(cl_int), &count_ext,
                &err
            );
            clCheckErrorMsg(err, "Failed to create count_d");

            const cl_int _batch_size = static_cast<cl_int>(batch_size * sizeof(T));

            cl_uint argi = 1;
            clCheckError(clSetKernelArg(kernel, argi++, sizeof(batch_d),     &batch_d));
            clCheckError(clSetKernelArg(kernel, argi++, sizeof(_batch_size), &_batch_size));
            clCheckError(clSetKernelArg(kernel, argi++, sizeof(count_d),     &count_d));
            clCheckError(clSetKernelArg(kernel, argi++, sizeof(*eos_d),      eos_d));
        }

        void execute()
        {
            clCheckError(clEnqueueTask(queue, kernel, 0, nullptr, &kernel_event));
            // clFlush(queue);

            cl_mem buffers[] = {batch_d, count_d, *eos_d};
            clCheckError(clEnqueueMigrateMemObjects(
                queue, 3, buffers,
                CL_MIGRATE_MEM_OBJECT_HOST,
                1, &kernel_event, &migrate_event
            ));
            // clFlush(queue);
        }

        void wait()
        {
            clCheckError(clWaitForEvents(1, &migrate_event));
            clCheckError(clReleaseEvent(kernel_event));
            clCheckError(clReleaseEvent(migrate_event));
        }

        ~Sink_Execution() {
            clReleaseMemObject(batch_d);
            clReleaseMemObject(count_d);
            clReleaseKernel(kernel);

            free(count_h);
            free(batch_h);
        }
    };

template <typename T>
struct Sink
{
    OCL & ocl;

    size_t max_batch_size;
    size_t number_of_buffers;
    size_t iterations;

    cl_command_queue queue;

    cl_mem eos_d;
    cl_int * eos;

    std::deque<Sink_Execution<T> *> waiting_executions;
    std::deque<Sink_Execution<T> *> running_executions;

    Sink(OCL & ocl_,
        const size_t batch_size,
        const size_t N)
    : ocl(ocl_)
    , max_batch_size(next_pow2(batch_size))
    , number_of_buffers(N)
    , iterations(0)
    {
        if (batch_size != max_batch_size) {
            std::cout << "fx::Sink: `batch_size` is rounded to the next power of 2 ("
                      << batch_size << " -> " << max_batch_size << ")" << '\n';
        }

        cl_int err;

        queue = ocl.createCommandQueue();

        eos = aligned_alloc<cl_int>(1);
        *eos = 0;

        cl_mem_ext_ptr_t eos_ext;
        eos_ext.obj = eos;
        eos_ext.param = 0;
        eos_ext.flags = pc_[31];

        eos_d = clCreateBuffer(
            ocl.context,
            CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
            sizeof(cl_int), &eos_ext,
            &err
        );
        clCheckErrorMsg(err, "Failed to create eos_d");

        clCheckError(clEnqueueWriteBuffer(
            queue, eos_d, CL_TRUE,
            0, sizeof(cl_int), eos,
            0, nullptr, nullptr
        ));

        for (size_t n = 0; n < number_of_buffers; ++n) {
            waiting_executions.push_back(new Sink_Execution<T>(ocl, queue, max_batch_size, &eos_d));
        }

        for (size_t n = 0; n < number_of_buffers; ++n) {
            launch_kernel();
        }
    }

    void launch_kernel()
    {
        Sink_Execution<T> * execution = waiting_executions.front();
        waiting_executions.pop_front();
        execution->execute();
        running_executions.push_back(execution);
    }

    T * pop(
        size_t * count,
        bool * last)
    {
        if (running_executions.empty()) {
            std::cerr << "fx::Sink: no launched executions" << '\n';
            exit(1);
        }

        Sink_Execution<T> * execution = running_executions.front();
        running_executions.pop_front();
        execution->wait();

        T * batch = execution->batch_h;
        *count = execution->count_h[0];
        *last = eos[0];

        waiting_executions.push_back(execution);

        iterations++;

        return batch;
    }

    void put_batch(T * batch)
    {
        if (!batch) {
            std::cerr << "fx::Source: batch is nullptr" << '\n';
            exit(1);
        }

        launch_kernel();
    }

    void launch_kernels() {}

    void finish()
    {
        while (!running_executions.empty()) {
            Sink_Execution<T> * execution = running_executions.front();
            running_executions.pop_front();
            execution->wait();
            waiting_executions.push_back(execution);
        }

        clFinish(queue);
    }

    ~Sink()
    {
        finish();

        while (!waiting_executions.empty()) {
            Sink_Execution<T> * execution = waiting_executions.front();
            waiting_executions.pop_front();
            delete execution;
        }

        while (!running_executions.empty()) {
            Sink_Execution<T> * execution = running_executions.front();
            running_executions.pop_front();
            delete execution;
        }

        clCheckError(clReleaseCommandQueue(queue));

        clReleaseMemObject(eos_d);
        free(eos);
    }
};
}

void source_thread(
    OCL & ocl,
    std::vector<record_t> & dataset,
    size_t n,
    size_t batch_size,
    size_t max_keys,
    size_t iterations
)
{
#if 1
    fx::Source<record_t> source(ocl, batch_size, n);

    std::cout << "Waiting for barrier..." << '\n';
    pthread_barrier_wait(&barrier);

    auto timestart_source = get_time();
    for (size_t it = 0; it < iterations; ++it) {
        record_t * batch = source.get_batch();
        // auto fill_timestart = get_time();
        fill_batch_with_dataset(dataset, batch, batch_size);
        // auto fill_timeend = get_time();
        // print_time<record_t>("fill_batch", batch_size, fill_timestart, fill_timeend);

        // std::cout << "Source: " << it << " push()" <<  '\n';
        source.push(batch, batch_size, (it == (iterations - 1)));
    }
    source.finish();
    auto timeend_source = get_time();
    print_time<record_t>("source", iterations * batch_size, timestart_source, timeend_source);
#else

    fx::SourceTask<record_t> source_task(ocl, batch_size, n);

    std::cout << "Waiting for barrier..." << '\n';
    pthread_barrier_wait(&barrier);

    auto timestart_source = get_time();
    for (size_t it = 0; it < iterations; ++it) {
        fx::Source_Execution<record_t> * execution = source_task.get_task();
        record_t * batch = execution->batch_h;
        fill_batch_with_dataset(dataset, batch, batch_size);
        source_task.push(execution, (it == (iterations - 1)));
    }
    source_task.finish();
    auto timeend_source = get_time();
    print_time<record_t>("source", iterations * batch_size, timestart_source, timeend_source);
#endif
}

void sink_thread(
    OCL & ocl,
    size_t batch_size,
    size_t n
)
{
    fx::Sink<record_t> sink(ocl, batch_size, n);

    std::cout << "Waiting for barrier..." << '\n';
    pthread_barrier_wait(&barrier);

    size_t it = 0;
    bool done = false;
    auto timestart_sink = get_time();
    while(!done) {

        size_t out_count;
        bool eos;
        record_t * batch = sink.pop(&out_count, &eos);
        size_t real_count = out_count * ((512 / 8) / sizeof(record_t));

        std::cout
            << "sink (" << it
            << ", " << real_count
            << ", " << (eos == 0 ? false: true)
            <<  ")"
            << '\n';

        sink.put_batch(batch);

        // for (size_t j = 0; j < real_count; ++j) {
        //     print_record(batch[j]);
        // }

        done = (eos == 0 ? false : true);
        it++;
    }

    sink.finish();

    _n_sink = it;
    auto timeend_sink = get_time();
    print_time<record_t>("sink", it * batch_size, timestart_sink, timeend_sink);
}

int main(int argc, char** argv) {

    argc--;
    argv++;

    std::string bitstreamFilename  = "./sd.xclbin";
    size_t iterations = 1;
    size_t n_source = 1;
    size_t n_sink = 1;
    size_t source_batch_size = 1 << 5;
    size_t sink_batch_size = 1 << 5;
    size_t max_keys = record_t::MAX_KEY_VALUE;

    int argi = 0;
    if (argc > argi) bitstreamFilename  = argv[argi++];
    if (argc > argi) iterations         = atoi(argv[argi++]);
    if (argc > argi) n_source           = atoi(argv[argi++]);
    if (argc > argi) n_sink             = atoi(argv[argi++]);
    if (argc > argi) source_batch_size  = atoi(argv[argi++]);
    if (argc > argi) sink_batch_size    = atoi(argv[argi++]);
    if (argc > argi) max_keys           = atoi(argv[argi++]);

    size_t data_transfer_size = (sizeof(record_t) * n_source * source_batch_size) / 1024;

    std::cout
        << COUT_HEADER << "bitstream: "        <<                 bitstreamFilename  << "\n"
        << COUT_HEADER << "iterations"         << COUT_INTEGER << iterations         << "\n"
        << COUT_HEADER << "n_source"           << COUT_INTEGER << n_source           << "\n"
        << COUT_HEADER << "n_sink"             << COUT_INTEGER << n_sink             << "\n"
        << COUT_HEADER << "source_batch_size"  << COUT_INTEGER << source_batch_size  << "\n"
        << COUT_HEADER << "sink_batch_Size"    << COUT_INTEGER << sink_batch_size    << "\n"
        << COUT_HEADER << "data_transfer_size" << COUT_INTEGER << data_transfer_size << " KB\n"
        << '\n';


    OCL ocl = OCL(bitstreamFilename, 0, 0, true);

    std::vector<record_t> dataset = get_dataset<record_t>("dataset.dat", TEMPERATURE);
    std::cout << dataset.size() << " tuples loaded!" << '\n';

    pthread_barrier_init(&barrier, nullptr, 2);

    auto timeStart = std::chrono::high_resolution_clock::now();

    std::thread th_source(
        source_thread,
        std::ref(ocl),
        std::ref(dataset),
        n_source,
        source_batch_size,
        max_keys,
        iterations
    );

    std::thread th_sink(
        sink_thread,
        std::ref(ocl),
        sink_batch_size,
        n_sink
    );

    th_source.join();
    th_sink.join();

    auto timeEnd = std::chrono::high_resolution_clock::now();

    // std::cout << COUT_HEADER << "n_sink: " << COUT_INTEGER << n_sink << '\n';
    print_time<record_t>("overall(source)", iterations * source_batch_size, timeStart, timeEnd);
    print_time<record_t>("overall(sink)", _n_sink * sink_batch_size, timeStart, timeEnd);
    print_time<record_t>("overall", iterations * source_batch_size, timeStart, timeEnd, false);

    pthread_barrier_destroy(&barrier);

    ocl.clean();

    bool match = true;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
