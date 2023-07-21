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
#include "common/constants.hpp"
#include "tuples/record_t.hpp"
#include "dataset/dataset.hpp"


#define DEBUG_MR_PRINT_FILL_BW         0
#define DEBUG_MR_SKIP_COPY             0
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
        << COUT_HEADER_SMALL << "\tTime: " << COUT_FLOAT_(2) << time_diff_ns / 1e6 << " ms"
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
    std::lock_guard<std::mutex> lock(mutex_print);

    std::cout << "("
    << r.key << ", "
    << r.property_value << ", "
    << r.incremental_average << ", "
    << r.timestamp
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

template <typename T>
size_t fill_batch_with_dataset(
    const std::vector<T, fx::aligned_allocator<T>> & dataset,
    T * batch,
    const size_t size,
    const size_t offset = 0
)
{
    size_t total_copied = 0;

    size_t n = size / dataset.size();
    size_t remaining_elems = size % dataset.size();
    size_t remaining = dataset.size() - offset;

    T * bufferStart = batch;
    if (offset > 0 && n > 0) {

        if (remaining > size) {
            memcpy(bufferStart, dataset.data() + offset, size * sizeof(T));
            total_copied += size;
            return size;
        }

        memcpy(bufferStart, dataset.data() + offset, remaining * sizeof(T));
        bufferStart += remaining;

        total_copied += remaining;

        n = (size - remaining) / dataset.size();
        remaining_elems = (size - remaining) % dataset.size();
    }

    for (size_t i = 0; i < n; i++) {
        memcpy(bufferStart, dataset.data(), dataset.size() * sizeof(T));
        bufferStart += dataset.size();
        total_copied += dataset.size();
    }

    if (remaining_elems > 0) {
        memcpy(bufferStart, dataset.data(), remaining_elems * sizeof(T));
        total_copied += remaining_elems;
    }

    return total_copied;
}

void mr_thread(
    const size_t idx,
    fx::OCL & ocl,
    const std::vector<record_t, fx::aligned_allocator<record_t>> & dataset,
    const size_t iterations,
    const size_t num_batches,
    const size_t batch_size
)
{
    fx::MemoryReader<record_t> mr(ocl, batch_size, num_batches, idx);
    size_t _tuples_sent = 0;
    size_t _batch_sent = 0;

    pthread_barrier_wait(&barrier);

    size_t tuple_idx = 0;

    auto time_fill = 0;
    const auto time_start = get_time();
    for (size_t it = 0; it < iterations; ++it) {
        record_t * batch = mr.get_batch();

        const auto fill_time_start = get_time();
#if LOAD_REAL_DATASET
        size_t _s = fill_batch_with_dataset(dataset, batch, batch_size, tuple_idx);
        tuple_idx = (tuple_idx + _s) % dataset.size();
        if (_s != batch_size) {
            std::cout << "MR " << idx << ": fill_batch_with_dataset returned " << _s << " instead of " << batch_size << '\n';
        }
#else
        // memcpy(batch, dataset.data(), batch_size * sizeof(record_t));
#if DEBUG_MR_SKIP_COPY
        if (it < num_batches) {
            std::copy(dataset.begin(), dataset.begin() + batch_size, batch);
        }
#else
        std::copy(dataset.begin(), dataset.begin() + batch_size, batch);
#endif

#endif
        const auto fill_time_end = get_time();
        time_fill += std::chrono::duration_cast<std::chrono::microseconds>(fill_time_end - fill_time_start).count();

#if DEBUG_MR_PRINT_FILL_BW
        print_time<record_t>("MR " + std::to_string(idx) + ": fill", batch_size, fill_time_start, fill_time_end);
#endif

        mr.push(batch, batch_size, (it == (iterations - 1)));
        _tuples_sent += batch_size;
        _batch_sent++;
    }

    mr.finish();
    const auto time_end = get_time();

    tuples_sent += _tuples_sent;
    batches_sent += _batch_sent;

    auto time_diff_mr = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
    auto time_fill_perc = time_fill * 100.0 / time_diff_mr;
    std::cout
        << "MR " << idx
        << " fill time: " << (time_fill / 1.e3)
        << " ms over " << (time_diff_mr / 1.e3)
        << " ms (" << time_fill_perc << "%)"
        << '\n';

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
    fx::MemoryWriter<record_t> mw(ocl, batch_size, num_batches, idx);
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

#if DEBUG_MW_PRINT_TUPLES
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
    std::cout << "MW " << idx << ": received " << _tuples_received << " tuples in " << _batch_received << " batches" << '\n';
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


    // print the current directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "Current working dir: " << cwd << std::endl;
    } else {
        perror("getcwd() error");
        return 1;
    }

    std::vector<std::vector<record_t, fx::aligned_allocator<record_t>>> datasets;
    for (size_t i = 0; i < mr_num_threads; ++i) {
        #if LOAD_REAL_DATASET
            datasets.push_back(get_dataset<record_t>("dataset/dataset.dat", TEMPERATURE));
        #else
            datasets.push_back(generate_dataset(mr_batch_size, i));
        #endif
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