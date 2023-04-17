#include "xcl2.hpp"

#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>
#include <array>
#include <deque>
#include <thread>
#include <string>
#include <pthread.h>

#define ALWAYS_INLINE           inline __attribute__((always_inline))
#define TEMPLATE_FLOATING       template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>

TEMPLATE_FLOATING
ALWAYS_INLINE bool approximatelyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
{
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}

#include "../common/tuple.hpp"
#include "sd_dataset.hpp"


#include <iomanip>

#define COUT_STRING_W           24
#define COUT_DOUBLE_W           8
#define COUT_INTEGER_W          8
#define COUT_PRECISION          3

#define COUT_HEADER             std::setw(COUT_STRING_W) << std::right
#define COUT_FLOAT              std::setw(COUT_DOUBLE_W) << std::right << std::fixed << std::setprecision(COUT_PRECISION)
#define COUT_INTEGER            std::setw(COUT_INTEGER_W) << std::right << std::fixed
#define COUT_BOOLEAN            std::boolalpha


size_t n_sink = 0;

pthread_barrier_t barrier;

template <typename T>
using Batch_t = std::vector<T, aligned_allocator<T>>;

#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }


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
            << std::endl;
    } else {
        double thr = number_of_tuples;
        thr /= timeDiff;
        thr *= 1000000;   // convert us to s
        size_t tuple_sec = thr / 1000; // convert to Kilo tuples/sec
        std::cout
            << COUT_HEADER << name
            << COUT_HEADER << "\tTime: "       << COUT_FLOAT   << timeDiff / 1000  << " ms"
            << COUT_HEADER << "\tKTuple/sec: " << COUT_INTEGER << tuple_sec
            << std::endl;
    }
}

auto get_time()
{
    return std::chrono::high_resolution_clock::now();
}

struct XilinxContext
{
    cl::Context context;
    cl::Program program;
    cl::Device device;
    cl::CommandQueue queue;

    XilinxContext(const std::string & bitstreamFilename)
    {
        cl_int err;

        auto devices = xcl::get_xil_devices();
        auto bins = xcl::import_binary_file(bitstreamFilename);

        bool valid_device = false;
        for (unsigned int i = 0; i < devices.size(); i++) {
            auto d = devices[i];
            OCL_CHECK(err, context = cl::Context(d, nullptr, nullptr, nullptr, &err));

            std::cout << "Trying to program device[" << i << "]: " << d.getInfo<CL_DEVICE_NAME>() << std::endl;
            program = cl::Program(context, {d}, bins, nullptr, &err);
            if (err != CL_SUCCESS) {
                std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
            } else {
                valid_device = true;
                device = d;
                break;
            }
        }
        if (!valid_device) {
            std::cout << "Failed to program any device found, exit!\n";
            exit(EXIT_FAILURE);
        }

        queue = cl::CommandQueue(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &err);
        OCL_CHECK(err, err);
    }

    cl::CommandQueue createQueue()
    {
        cl_int err;
        cl::CommandQueue q;
        OCL_CHECK(err, q = cl::CommandQueue(context, device, /*CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |*/ CL_QUEUE_PROFILING_ENABLE, &err));
        return q;
    }

    cl::Kernel createKernel(const std::string & kernelName)
    {
        cl_int err;
        cl::Kernel k;
        OCL_CHECK(err, k = cl::Kernel(program, kernelName.c_str(), &err));
        return k;
    }
};

template <typename T>
struct CLBuffers
{
    const XilinxContext & xcontext;
    size_t n;
    size_t batch_size;
    cl_mem_flags flags;

    std::vector<Batch_t<T>> h_batches;
    std::vector<cl::Memory> d_batches;

    size_t index;

    CLBuffers(
        const XilinxContext & xcontext,
        size_t n,
        size_t batch_size,
        cl_mem_flags flags
    )
    : xcontext(xcontext)
    , n(n)
    , batch_size(batch_size)
    , flags(flags)
    , h_batches(n, Batch_t<T>(batch_size))
    , d_batches(n)
    , index(0)
    {
        cl_int err;
        for (int i = 0; i < n; ++i) {
            OCL_CHECK(err, d_batches[i] = cl::Buffer(xcontext.context,
                                                     flags | CL_MEM_USE_HOST_PTR,
                                                     sizeof(T) * h_batches[i].size(), h_batches[i].data(),
                                                     &err));
        }
    }

    cl::Event migrate(size_t i, std::vector<cl::Event> * events = nullptr, bool blocking = false, bool to_host = false)
    {
        cl_int err;
        cl::Event migrate_event;
        OCL_CHECK(err, err = xcontext.queue.enqueueMigrateMemObjects({d_batches[i]},
                                                                     (to_host ? CL_MIGRATE_MEM_OBJECT_HOST : 0),
                                                                     events, &migrate_event));
        if (blocking) {
            migrate_event.wait();
        }

        return migrate_event;
    }

    cl::Event migrateToDevice(size_t i, std::vector<cl::Event> * events = nullptr, bool blocking = false) { return migrate(i, events, blocking); }
    cl::Event migrateToHost(size_t i, std::vector<cl::Event> * events = nullptr, bool blocking = false) { return migrate(i, events, blocking, true); }
    cl::Event migrateToDevice(std::vector<cl::Event> * events = nullptr, bool blocking = false) { return migrate(index, events, blocking); }
    cl::Event migrateToHost(std::vector<cl::Event> * events = nullptr, bool blocking = false) { return migrate(index, events, blocking, true); }
    void next() { index = (index + 1) % n; }
    size_t currentIndex() { return index; }

    Batch_t<T> & getHostBuffer(size_t i) { return h_batches[i]; }
    cl::Memory getDeviceBuffer(size_t i) { return d_batches[i]; }

    Batch_t<T> & currentHostBuffer() { return h_batches[index]; }
    cl::Memory currentDeviceBuffer() { return d_batches[index]; }
};

double event_time_us(cl::Event e)
{
    cl_ulong start;
    cl_ulong end;
    e.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    e.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    return (end - start) / 1000.0;
}

static record_t new_record(int i)
{
    record_t r;
    r.key = static_cast<unsigned int>(i % record_t::MAX_KEY_VALUE);
    r.property_value = static_cast<float>(i);
    r.incremental_average = 0;
    r.timestamp = 0;
    return r;
}

static void print_record(const record_t & r)
{
    std::cout << "("
    << r.key << ", "
    << r.property_value << ", "
    << r.incremental_average << ", "
    << r.timestamp
    << ")"
    << std::endl;
}

void fill_batch(Batch_t<record_t> & batch)
{
    // unsigned int i = 0;
    // for (record_t & t : batch) {
    //     t.key = i;
    //     t.property_value = i;
    //     t.timestamp = i + 1;
    //     ++i;
    // }
    for (int i = 0; i < batch.size(); ++i) {
        batch[i] = new_record(i);
    }
}

// void source_thread(
//     XilinxContext & xcontext,
//     std::vector<record_t> & dataset,
//     size_t n,
//     size_t batch_size,
//     size_t max_keys,
//     size_t iterations
// )
// {
//     cl_int err;
//     cl::Kernel k = xcontext.createKernel("memory_reader");

//     auto timestart_fill = get_time();
//     CLBuffers<record_t> batches(xcontext, n, batch_size, CL_MEM_READ_ONLY);
//     for (int i = 0; i < n; ++i) {
//         fill_batch(batches.getHostBuffer(i));
//     }
//     auto timeend_fill = get_time();
//     print_time<record_t>("fill", n * batch_size, timestart_fill, timeend_fill);


//     // size_t t_index = 0;
//     // for (int i = 0; i < n; ++i) {
//     //     auto buff = batches.getHostBuffer(i);
//     //     for (int j = 0; j < batch_size; ++j) {
//     //         buff[j] = dataset[t_index];
//     //         t_index = (t_index + 1) % dataset.size();
//     //         buff[j].print();
//     //     }
//     // }

//     std::cout << "Waiting for barrier..." << std::endl;
//     pthread_barrier_wait(&barrier);

//     auto timestart_source = get_time();
//     std::vector<cl::Event> kernel_event(iterations);
//     for (int i = 0; i < iterations; ++i) {
//         std::vector<cl::Event> events;
//         if (i >= n) {
//             events.push_back(kernel_event[i - n]);
//         }

//         std::vector<cl::Event> wait_events;
//         wait_events.push_back(batches.migrateToDevice((events.size() > 0 ? &events : nullptr)));

//         int in_size = int(batches.currentHostBuffer().size() / (512 / (sizeof(record_t) * 8)));
//         int eos_int = (i == (iterations - 1) ? 1 : 0);
//         std::cout << "(source) in_size: " << in_size << std::endl;
//         std::cout << "(source) eos_int: " << eos_int << std::endl;

//         cl_int argi = 0;
//         OCL_CHECK(err, err = k.setArg(argi++, batches.currentDeviceBuffer()));
//         OCL_CHECK(err, err = k.setArg(argi++, in_size));
//         OCL_CHECK(err, err = k.setArg(argi++, eos_int));
//         OCL_CHECK(err, err = xcontext.queue.enqueueTask(k, &wait_events, &kernel_event[i]));

//         // 0 line_t * in,
//         // 1 int in_count,
//         // 2 int eos,
//         // 3 axis_stream_t & out

//         batches.next();
//     }

//     kernel_event[iterations - 1].wait();
//     auto timeend_source = get_time();
//     print_time<record_t>("source", iterations * batch_size, timestart_source, timeend_source);
// }

void source_thread(
    XilinxContext & xcontext,
    std::vector<record_t> & dataset,
    size_t n,
    size_t batch_size,
    size_t max_keys,
    size_t iterations
)
{
    cl_int err;
    cl::Kernel k = xcontext.createKernel("memory_reader");

    auto timestart_fill = get_time();
    CLBuffers<record_t> batches(xcontext, n, batch_size, CL_MEM_READ_ONLY);
    for (int i = 0; i < n; ++i) {
        fill_batch(batches.getHostBuffer(i));
    }
    auto timeend_fill = get_time();
    print_time<record_t>("fill", n * batch_size, timestart_fill, timeend_fill);


    // size_t t_index = 0;
    // for (int i = 0; i < n; ++i) {
    //     auto buff = batches.getHostBuffer(i);
    //     for (int j = 0; j < batch_size; ++j) {
    //         buff[j] = dataset[t_index];
    //         t_index = (t_index + 1) % dataset.size();
    //         buff[j].print();
    //     }
    // }

    std::cout << "Waiting for barrier..." << std::endl;
    pthread_barrier_wait(&barrier);

    auto timestart_source = get_time();
    std::vector<cl::Event> kernel_event(iterations);
    for (int i = 0; i < iterations; ++i) {

        std::vector<cl::Event> wait_events;
        wait_events.push_back(batches.migrateToDevice());

        int in_size = int(batches.currentHostBuffer().size() / (512 / (sizeof(record_t) * 8)));
        int eos_int = (i == (iterations - 1) ? 1 : 0);
        std::cout << "(source) in_size: " << in_size << std::endl;
        std::cout << "(source) eos_int: " << eos_int << std::endl;

        cl_int argi = 0;
        OCL_CHECK(err, err = k.setArg(argi++, batches.currentDeviceBuffer()));
        OCL_CHECK(err, err = k.setArg(argi++, in_size));
        OCL_CHECK(err, err = k.setArg(argi++, eos_int));
        OCL_CHECK(err, err = xcontext.queue.enqueueTask(k, &wait_events, &kernel_event[i]));

        kernel_event[i].wait();

        batches.next();
    }

    auto timeend_source = get_time();
    print_time<record_t>("source", iterations * batch_size, timestart_source, timeend_source);
}

void sink_thread(
    XilinxContext & xcontext,
    size_t batch_size
)
{
    cl_int err;
    cl::Kernel k = xcontext.createKernel("memory_writer");

    CLBuffers<record_t> batches(xcontext, 1, batch_size, CL_MEM_WRITE_ONLY);
    CLBuffers<int> written_counts(xcontext, 1, 1, CL_MEM_WRITE_ONLY);
    CLBuffers<int> eos(xcontext, 1, 1, CL_MEM_WRITE_ONLY);

    std::cout << "Waiting for barrier..." << std::endl;
    pthread_barrier_wait(&barrier);
    auto timestart_sink = get_time();

    cl_int out_size = int(batches.currentHostBuffer().size() * sizeof(record_t));
    std::cout << "(sink) out_size: " << out_size << std::endl;


    size_t it = 0;
    bool done = false;
    while(!done) {
        std::vector<cl::Event> kernel_event(1);
        cl_int argi = 1;
        OCL_CHECK(err, err = k.setArg(argi++, batches.currentDeviceBuffer()));
        OCL_CHECK(err, err = k.setArg(argi++, out_size));
        OCL_CHECK(err, err = k.setArg(argi++, written_counts.currentDeviceBuffer()));
        OCL_CHECK(err, err = k.setArg(argi++, eos.currentDeviceBuffer()));

    // 0 axis_stream_t & in,
    // 1 line_t * out,
    // 2 int out_count,
    // 3 int * written_count,
    // 4 int * eos
        std::cout << "(sink) enqueueTask()" << std::endl;
        OCL_CHECK(err, err = xcontext.queue.enqueueTask(k, nullptr, &kernel_event[0]));

        batches.migrateToHost(&kernel_event, true);
        written_counts.migrateToHost(&kernel_event, true);
        eos.migrateToHost(&kernel_event, true);

        int written_count = written_counts.currentHostBuffer()[0];
        int eos_bool = (eos.currentHostBuffer()[0] != 0);

        std::cout << "sink Elements: " << COUT_INTEGER << written_count << std::endl;
        std::cout << "     sink EOS: " << COUT_BOOLEAN << eos_bool << std::endl;

        for (int i = 0; i < batch_size; ++i) {
            print_record(batches.currentHostBuffer()[i]);
        }

        done = eos_bool;
        it++;
        n_sink++;
    }

    auto timeend_sink = get_time();
    print_time<record_t>("sink", it * batch_size, timestart_sink, timeend_sink);
}

int main(int argc, char** argv) {

    argc--;
    argv++;

    std::string bitstreamFilename  = "./sd.xclbin";
    size_t iterations = 1;
    size_t n = 1;
    size_t source_batch_size = 1 << 5;
    size_t sink_batch_size = 1 << 5;
    size_t max_keys = record_t::MAX_KEY_VALUE;

    int argi = 0;
    if (argc > argi) bitstreamFilename  = argv[argi++];
    if (argc > argi) iterations         = atoi(argv[argi++]);
    if (argc > argi) n                  = atoi(argv[argi++]);
    if (argc > argi) source_batch_size  = atoi(argv[argi++]);
    if (argc > argi) sink_batch_size    = atoi(argv[argi++]);
    if (argc > argi) max_keys           = atoi(argv[argi++]);

    size_t data_transfer_size = (sizeof(record_t) * n * source_batch_size) / 1024;

    std::cout
        << COUT_HEADER << "bitstream: "        <<                 bitstreamFilename  << "\n"
        << COUT_HEADER << "iterations"         << COUT_INTEGER << iterations         << "\n"
        << COUT_HEADER << "n"                  << COUT_INTEGER << n                  << "\n"
        << COUT_HEADER << "source_batch_size"  << COUT_INTEGER << source_batch_size  << "\n"
        << COUT_HEADER << "sink_batch_Size"    << COUT_INTEGER << sink_batch_size    << "\n"
        << COUT_HEADER << "data_transfer_size" << COUT_INTEGER << data_transfer_size << " KB\n"
        << std::endl;


    XilinxContext xcontext = XilinxContext(bitstreamFilename);

    std::vector<record_t> dataset = get_dataset<record_t>("dataset.dat", TEMPERATURE);
    std::cout << dataset.size() << " tuples loaded!" << std::endl;

    pthread_barrier_init(&barrier, NULL, 2);


    auto timeStart = std::chrono::high_resolution_clock::now();

    std::thread th_source(
        source_thread,
        std::ref(xcontext),
        std::ref(dataset),
        n,
        source_batch_size,
        max_keys,
        iterations
    );

    std::thread th_sink(
        sink_thread,
        std::ref(xcontext),
        sink_batch_size
    );

    // std::cout << "Waiting for barrier..." << std::endl;
    // pthread_barrier_wait(&barrier);

    th_source.join();
    th_sink.join();
    xcontext.queue.finish();

    auto timeEnd = std::chrono::high_resolution_clock::now();

    // std::cout << COUT_HEADER << "n_sink: " << COUT_INTEGER << n_sink << std::endl;
    print_time<record_t>("overall(source)", iterations * source_batch_size, timeStart, timeEnd);
    print_time<record_t>("overall(sink)", n_sink * sink_batch_size, timeStart, timeEnd);
    print_time<record_t>("overall", iterations * source_batch_size, timeStart, timeEnd, false);

    pthread_barrier_destroy(&barrier);

    bool match = true;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
