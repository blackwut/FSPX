#include "fx_opencl.hpp"

#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>
#include <array>
#include <deque>
#include <string>
#include <chrono>

#include "tuple.hpp"

int main(int argc, char** argv) {

    argc--;
    argv++;

    auto host_overallStart = std::chrono::high_resolution_clock::now();

    std::string bitstreamFilename  = "./compute.xclbin";
    OCL ocl = OCL(bitstreamFilename, 0, 0, true);

    // run a kernel and take its time, bandwidth
    cl_command_queue queue = ocl.createCommandQueue();
    cl_kernel kernel = ocl.createKernel("compute");

    cl_event event;
    auto host_kernelStart = std::chrono::high_resolution_clock::now();
    clEnqueueTask(queue, kernel, 0, NULL, &event);
    printf("Kernel enqueued\n");
    clFinish(queue);

    auto host_end = std::chrono::high_resolution_clock::now();

    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);

    constexpr unsigned int N = (1 << 31);
    double time_ns = (double)(end - start);
    double time_s = time_ns / 1000000000.0;

    double bandwidth_gbs = (double)(N * sizeof(record_t)) / time_ns;
    double tuplePerSecond = (double)(N) / time_s;
    double KTuplesPerSecond = tuplePerSecond / 1000.0;
    double MTuplesPerSecond = tuplePerSecond / 1000000.0;

    std::cout << "Time Host (overall): " << std::chrono::duration_cast<std::chrono::milliseconds>(host_end - host_overallStart).count() << " ms" << '\n';
    std::cout << "Time Host (kernel): " << std::chrono::duration_cast<std::chrono::milliseconds>(host_end - host_kernelStart).count() << " ms" << '\n';
    std::cout << "Time: " << time_s << " s" << '\n';
    std::cout << "Bandwidth: " << bandwidth_gbs << " GB/s" << '\n';
    std::cout << "Tuples per second: " << tuplePerSecond << '\n';
    std::cout << "KTuples per second: " << KTuplesPerSecond << '\n';
    std::cout << "MTuples per second: " << MTuplesPerSecond << '\n';

    ocl.clean();

    bool match = true;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
