#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <fstream>


#include "fspx_host.hpp"
#include "tuple.hpp"


#define DEBUG_MR_PRINT_FILL_BW         0
#define DEBUG_MW_PRINT_TUPLES          0
#define DEBUG_MW_NUM_TUPLES_TO_PRINT   4

std::mutex mutex_print;
pthread_barrier_t barrier;
std::atomic<size_t> tuples_sent;
std::atomic<size_t> tuples_received;
std::atomic<size_t> batches_sent;
std::atomic<size_t> batches_received;


auto get_time() { return std::chrono::high_resolution_clock::now(); }

template <typename T>
void print_time(
    std::string name,
    size_t num_tuples,
    double time_diff_ns,
    std::string color = fx::COLOR_WHITE
)
{
    std::lock_guard<std::mutex> lock(mutex_print);

    size_t throughput_tps = (num_tuples / time_diff_ns) * 1e9;
    size_t throughput_bs = (num_tuples * sizeof(T) / time_diff_ns) * 1e9;

    std::cout
        << COUT_HEADER_SMALL << name << ": "
        << COUT_HEADER_SMALL << "\tTime: "       << COUT_FLOAT_(2) << time_diff_ns / 1e6 << " ms"
        << COUT_HEADER_SMALL << "\tThroughput: "
        << fx::colorString(fx::formatTuplesPerSecond(throughput_tps), color)
        << fx::colorString(fx::formatBytesPerSecond(throughput_bs), color)
        << '\n';
}

template <typename T>
void print_time(
    std::string name,
    size_t num_tuples,
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end,
    std::string color = fx::COLOR_WHITE
)
{
    auto time_diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    print_time<T>(name, num_tuples, time_diff, color);
}


void print_record(const record_t & r)
{
    std::cout << "("
    << r.key << ", "
    << r.val << ", "
    << ")"
    << '\n';
}

void print_batch(
    record_t * batch,
    size_t size
)
{
    for (size_t i = 0; i < size; ++i) {
        print_record(batch[i]);
    }
}

std::vector<record_t> generate_dataset(size_t size)
{
    std::vector<record_t> dataset(size);
    for (size_t i = 0; i < size; ++i) {
        dataset[i].key = static_cast<unsigned int>(i % record_t::MAX_KEY_VALUE);
        dataset[i].val = static_cast<unsigned int>(i);
    }
    return dataset;
}

template <typename T>
void fill_batch_with_dataset(
    const std::vector<T> & dataset,
    T * batch,
    const size_t size
)
{
    size_t n = size / dataset.size();
    size_t remaining_elems = size % dataset.size();

    T * bufferStart = batch;
    for (size_t i = 0; i < n; i++) {
        memcpy(bufferStart, dataset.data(), dataset.size() * sizeof(T));
        bufferStart += dataset.size();
    }

    if (remaining_elems > 0) {
        memcpy(bufferStart, dataset.data(), remaining_elems * sizeof(T));
    }
}

void mr_thread(
    const size_t idx,
    fx::OCL & ocl,
    const std::vector<record_t> & dataset,
    const size_t iterations,
    const size_t num_batches,
    const size_t batch_size
)
{
    fx::MemoryReader<record_t> mr(ocl, batch_size, num_batches, idx + 1);
    size_t _tuples_sent = 0;
    size_t _batch_sent = 0;

    pthread_barrier_wait(&barrier);

    // std::this_thread::sleep_for(std::chrono::milliseconds(5 * idx));

    const auto time_start = get_time();
    for (size_t it = 0; it < iterations; ++it) {
        record_t * batch = mr.get_batch();

#if DEBUG_MR_PRINT_FILL_BW
        const auto fill_time_start = get_time();
        fill_batch_with_dataset(dataset, batch, batch_size);
        const auto fill_time_end = get_time();
        print_time<record_t>("MR " + std::to_string(idx) + ": fill", batch_size, fill_time_start, fill_time_end);
#else
        fill_batch_with_dataset(dataset, batch, batch_size);
#endif

        mr.push(batch, batch_size, (it == (iterations - 1)));
        _tuples_sent += batch_size;
        _batch_sent++;
    }

    mr.finish();
    const auto time_end = get_time();

    tuples_sent += _tuples_sent;
    batches_sent += _batch_sent;

    print_time<record_t>("MR " + std::to_string(idx), _tuples_sent, time_start, time_end);
    // print_time<record_t>("MR " + std::to_string(idx) + "(precise)", iterations * batch_size, mr.get_start_time(), mr.get_end_time());
}

void mw_thread(
    const size_t idx,
    fx::OCL & ocl,
    const size_t num_batches,
    const size_t batch_size
)
{
    fx::MemoryWriter<record_t> mw(ocl, batch_size, num_batches, idx + 1);
    size_t _tuples_received = 0;
    size_t _batch_received = 0;

    pthread_barrier_wait(&barrier);
    mw.prelaunch();

    bool done = false;
    const auto time_start = get_time();
    while(!done) {
        size_t count = 0;
        record_t * batch = mw.pop(&count, &done);

        size_t num_tuples = count * ((512 / 8) / sizeof(record_t));

#if DEBUG_MW_PRINT_BATCH
        std::cout
            << "MW (" << _batch_received
            << ", " << num_tuples
            << ", " << done
            <<  ")\n";

           for (size_t i = 0; i < DEBUG_MW_NUM_TUPLES_TO_PRINT; ++i) {
                print_record(batch[i]);
            }
#endif

        mw.put_batch(batch, batch_size);
        _tuples_received += num_tuples;
        _batch_received++;
    }

    mw.finish();
    const auto time_end = get_time();

    tuples_received += _tuples_received;
    batches_received += _batch_received;
    print_time<record_t>("MW " + std::to_string(idx), _tuples_received, time_start, time_end);
}

int main(int argc, char** argv) {

    argc--;
    argv++;

    std::string bitstream  = "./kernels/th111/hw/th111.xclbin";
    size_t iterations = 1;
    size_t mr_num_threads = 1;
    size_t mw_num_threads = 1;
    size_t mr_num_buffers = 1;
    size_t mw_num_buffers = 1;
    size_t mr_batch_size = 1 << 5;
    size_t mw_batch_size = 1 << 5;

    int argi = 0;
    if (argc > argi) bitstream      = argv[argi++];
    if (argc > argi) iterations     = atoi(argv[argi++]);
    if (argc > argi) mr_num_threads = atoi(argv[argi++]);
    if (argc > argi) mw_num_threads = atoi(argv[argi++]);
    if (argc > argi) mr_num_buffers = atoi(argv[argi++]);
    if (argc > argi) mw_num_buffers = atoi(argv[argi++]);
    if (argc > argi) mr_batch_size  = atoi(argv[argi++]);
    if (argc > argi) mw_batch_size  = atoi(argv[argi++]);

    const size_t transfer_size = sizeof(record_t) * mr_batch_size / 1024;

    std::cout
        << COUT_HEADER << "bitstream: "      <<                 bitstream      << "\n"
        << COUT_HEADER << "iterations: "     << COUT_INTEGER << iterations     << "\n"
        << COUT_HEADER << "mr_num_threads: " << COUT_INTEGER << mr_num_threads << "\n"
        << COUT_HEADER << "mw_num_threads: " << COUT_INTEGER << mw_num_threads << "\n"
        << COUT_HEADER << "mr_num_buffers: " << COUT_INTEGER << mr_num_buffers << "\n"
        << COUT_HEADER << "mw_num_buffers: " << COUT_INTEGER << mw_num_buffers << "\n"
        << COUT_HEADER << "mr_batch_size: "  << COUT_INTEGER << mr_batch_size  << "\n"
        << COUT_HEADER << "mw_batch_size: "  << COUT_INTEGER << mw_batch_size  << "\n"
        << COUT_HEADER << "transfer_size: "  << COUT_INTEGER << transfer_size  << " KB\n"
        << '\n';

    std::vector<std::vector<record_t>> datasets;
    for (size_t i = 0; i < mr_num_threads; ++i) {
        datasets.push_back(generate_dataset(mr_batch_size));
    }

    pthread_barrier_init(&barrier, nullptr, mr_num_threads + mw_num_threads + 1); // +1 for main thread
    tuples_sent = 0;
    batches_sent = 0;
    tuples_received = 0;
    batches_received = 0;


    auto time_start = get_time();

    fx::OCL ocl = fx::OCL(bitstream, 0, 0, true);

    std::vector<std::thread> mr_threads;
    for (size_t i = 0; i < mr_num_threads; ++i) {
        mr_threads.push_back(
            std::thread(
                mr_thread,
                i,
                std::ref(ocl),
                std::ref(datasets[i]),
                iterations,
                mr_num_buffers,
                mr_batch_size
            )
        );
    }

    std::vector<std::thread> mw_threads;
    for (size_t i = 0; i < mw_num_threads; ++i) {
        mw_threads.push_back(
            std::thread(
                mw_thread,
                i,
                std::ref(ocl),
                mw_num_buffers,
                mw_batch_size
            )
        );
    }

    pthread_barrier_wait(&barrier);

    auto time_start_compute = get_time();

    for (auto& t : mr_threads) t.join();
    for (auto& t : mw_threads) t.join();

    auto time_end = get_time();


    auto time_elapsed_compute_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_start_compute).count();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    print_time<record_t>("MR (compute)", tuples_sent, time_start_compute, time_end, fx::COLOR_LIGHT_GREEN);
    print_time<record_t>("MW (compute)", tuples_received, time_start_compute, time_end, fx::COLOR_LIGHT_GREEN);
    print_time<record_t>("APP (compute)", tuples_received, time_start_compute, time_end, fx::COLOR_LIGHT_GREEN);
    print_time<record_t>("MR (overall)", tuples_sent, time_start, time_end);
    print_time<record_t>("MW (overall)", tuples_received, time_start, time_end);
    print_time<record_t>("APP (overall)", tuples_sent, time_start, time_end);
    std::cout << COUT_HEADER_SMALL << "Total HOST time: " << COUT_INTEGER << time_elapsed << " ms\n";

    pthread_barrier_destroy(&barrier);
    ocl.clean();

    size_t tuples_sec = tuples_sent / (time_elapsed_compute_ns / 1e9);
    size_t bytes_sec = tuples_sec * sizeof(record_t);

    // dump results to file in append mode
    // if the file is empty or not exists, print the header
    bool print_header = false;
    std::ifstream infile("results.csv");
    if (!infile.good()) {
        print_header = true;
    }
    infile.close();

    std::ofstream outfile("results.csv", std::ios_base::app);
    if (print_header) {
        outfile << "bitstream,"
                << "iterations,"
                << "mr_num_threads,"
                << "mw_num_threads,"
                << "mr_num_buffers,"
                << "mw_num_buffers,"
                << "mr_batch_size,"
                << "mw_batch_size,"
                << "transfer_size,"
                << "time_elapsed_ms,"
                << "time_elapsed_compute_ns,"
                << "tuples_sent,"
                << "batches_sent,"
                << "tuples_received,"
                << "batches_received,"
                << "tuples_sec,"
                << "bytes_sec\n";
    }

    outfile << bitstream << ","
            << iterations << ","
            << mr_num_threads << ","
            << mw_num_threads << ","
            << mr_num_buffers << ","
            << mw_num_buffers << ","
            << mr_batch_size << ","
            << mw_batch_size << ","
            << transfer_size << ","
            << time_elapsed << ","
            << time_elapsed_compute_ns << ","
            << tuples_sent << ","
            << batches_sent << ","
            << tuples_received << ","
            << batches_received << ","
            << tuples_sec << ","
            << bytes_sec << "\n";
    outfile.close();

    bool match = true;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}