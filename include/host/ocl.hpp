#ifndef __HOST_OCL__
#define __HOST_OCL__

#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#define CL_TARGET_OPENCL_VERSION 120
#include "CL/opencl.h"
// #include "CL/cl_ext_xilinx.h"
#include "/opt/xilinx/xrt/include/CL/cl_ext_xilinx.h"

#include "utils.hpp"

namespace fx {

//*****************************************************************
//
// Error Handling
//
//*****************************************************************

static inline const char * clErrorToString(cl_int err) {
    switch (err) {
        case CL_SUCCESS                                  : return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND                         : return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE                     : return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE                   : return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE            : return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES                         : return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY                       : return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE             : return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP                         : return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH                    : return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED               : return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE                    : return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE                              : return "CL_MAP_FAILURE";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET             : return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case CL_COMPILE_PROGRAM_FAILURE                  : return "CL_COMPILE_PROGRAM_FAILURE";
        case CL_LINKER_NOT_AVAILABLE                     : return "CL_LINKER_NOT_AVAILABLE";
        case CL_LINK_PROGRAM_FAILURE                     : return "CL_LINK_PROGRAM_FAILURE";
        case CL_DEVICE_PARTITION_FAILED                  : return "CL_DEVICE_PARTITION_FAILED";
        case CL_KERNEL_ARG_INFO_NOT_AVAILABLE            : return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
        case CL_INVALID_VALUE                            : return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE                      : return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM                         : return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE                           : return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT                          : return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES                 : return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE                    : return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR                         : return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT                       : return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR          : return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE                       : return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER                          : return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY                           : return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS                    : return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM                          : return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE               : return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME                      : return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION                : return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL                           : return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX                        : return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE                        : return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE                         : return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS                      : return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION                   : return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE                  : return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE                   : return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET                    : return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST                  : return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT                            : return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION                        : return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT                        : return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE                      : return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL                        : return "CL_INVALID_MIP_LEVEL";
        case CL_INVALID_GLOBAL_WORK_SIZE                 : return "CL_INVALID_GLOBAL_WORK_SIZE";
        case CL_INVALID_PROPERTY                         : return "CL_INVALID_PROPERTY";
        case CL_INVALID_IMAGE_DESCRIPTOR                 : return "CL_INVALID_IMAGE_DESCRIPTOR";
        case CL_INVALID_COMPILER_OPTIONS                 : return "CL_INVALID_COMPILER_OPTIONS";
        case CL_INVALID_LINKER_OPTIONS                   : return "CL_INVALID_LINKER_OPTIONS";
        case CL_INVALID_DEVICE_PARTITION_COUNT           : return "CL_INVALID_DEVICE_PARTITION_COUNT";
        default                                          : return "CL_UNKNOWN_ERROR";
    }
}

#define clCheckError(status) _clCheckError(__FILE__, __LINE__, status, #status)
#define clCheckErrorMsg(status, message) _clCheckError(__FILE__, __LINE__, status, message)

void _clCheckError(const char * file, int line,
                   cl_int error, const std::string & message)
{
    if (error != CL_SUCCESS) {
        std::cerr << "\n"
                  << "ERROR: " << clErrorToString(error) << "\n"
                  << "Location: " << file << ":" << line << "\n"
                  << "Message: " << message << "\n"
                  << '\n';
        exit(error);
    }
}


//*****************************************************************
//
// OpenCL Platform
//
//*****************************************************************

void clCallback(const char * errinfo, const void *, size_t, void *)
{
    std::cerr << "Context callback: " << errinfo << '\n';
}

std::vector<cl_platform_id> clGetPlatforms()
{
    cl_uint num_platforms;
    clCheckError(clGetPlatformIDs(0, NULL, &num_platforms));

    std::vector<cl_platform_id> platforms(num_platforms);
    clCheckError(clGetPlatformIDs(num_platforms, platforms.data(), NULL));

    return platforms;
}

std::string platformInfo(cl_platform_id platform, cl_platform_info info)
{
    size_t size;
    clCheckError(clGetPlatformInfo(platform, info, 0, NULL, &size));
    std::vector<char> param_value(size);
    clCheckError(clGetPlatformInfo(platform, info, size, param_value.data(), NULL));

    return std::string(param_value.begin(), param_value.end());
}

cl_platform_id clSelectPlatform(int selected_platform)
{
    const auto platforms = clGetPlatforms();
    if (selected_platform < 0 or size_t(selected_platform) >= platforms.size()) {
        std::cerr << "ERROR: Platform #" << selected_platform << " does not exist\n";
        exit(-1);
    }

    return platforms[selected_platform];
}

cl_platform_id clPromptPlatform()
{
    const auto platforms = clGetPlatforms();

    int selected_platform = -1;
    while (selected_platform < 0 or size_t(selected_platform) >= platforms.size()) {
        int platform_id = 0;
        for (const auto & p : platforms) {
            std::cout << "#" << platform_id++                  << " "
                      << platformInfo(p, CL_PLATFORM_NAME)     << " "
                      << platformInfo(p, CL_PLATFORM_VENDOR)   << " "
                      << platformInfo(p, CL_PLATFORM_VERSION)
                      << '\n';
        }

        std::cout << "\nSelect a platform: " << std::flush;
        std::cin >> selected_platform;
        std::cout << '\n';
    }

    return platforms[selected_platform];
}


//*****************************************************************
//
// OpenCL Device
//
//*****************************************************************

std::vector<cl_device_id> clGetDevices(cl_platform_id platform,
                                       cl_device_type device_type)
{
    cl_uint num_devices;
    clCheckError(clGetDeviceIDs(platform, device_type, 0, NULL, &num_devices));

    std::vector<cl_device_id> devices(num_devices);
    clCheckError(clGetDeviceIDs(platform, device_type, num_devices, devices.data(), NULL));

    return devices;
}

template <typename T>
T deviceInfo(cl_device_id device, cl_device_info info)
{
    size_t size;
    clCheckError(clGetDeviceInfo(device, info, 0, NULL, &size));

    T param_value;
    clCheckError(clGetDeviceInfo(device, info, size, &param_value, NULL));
    return param_value;
}

template <>
std::string deviceInfo<std::string>(cl_device_id device, cl_device_info info)
{
    size_t size;
    clCheckError(clGetDeviceInfo(device, info, 0, NULL, &size));

    std::vector<char> param_value;
    clCheckError(clGetDeviceInfo(device, info, size, param_value.data(), NULL));
    return std::string(param_value.begin(), param_value.end());
}

cl_device_id clSelectDevice(cl_platform_id platform, int selected_device)
{
    const auto devices = clGetDevices(platform, CL_DEVICE_TYPE_ALL);
    if (selected_device < 0 or size_t(selected_device) >= devices.size()) {
        std::cerr << "ERROR: Device #" << selected_device << "does not exist\n";
        exit(-1);
    }

    return devices[selected_device];
}

cl_device_id clPromptDevice(cl_platform_id platform)
{
    const auto devices = clGetDevices(platform, CL_DEVICE_TYPE_ALL);

    auto deviceTypeName = [](const cl_device_type type) {
        switch (type) {
            case CL_DEVICE_TYPE_CPU:            return "CPU";
            case CL_DEVICE_TYPE_GPU:            return "GPU";
            case CL_DEVICE_TYPE_ACCELERATOR:    return "ACCELERATOR";
            default:                            return "NONE";
        }
    };

    int selected_device = -1;
    while (selected_device < 0 or size_t(selected_device) >= devices.size()) {
        int device_id = 0;
        for (const auto & d : devices) {
            std::cout << "#" << device_id++
                      << " [" << deviceTypeName(deviceInfo<cl_device_type>(d, CL_DEVICE_TYPE)) << "] "
                      // << deviceInfo<std::string>(d, CL_DEVICE_NAME) << "\n"
                      // << "\tVendor:            " <<  deviceInfo<std::string>(d, CL_DEVICE_VENDOR)                 << "\n"
                      // << "\tMax Compute Units: " <<  deviceInfo<cl_uint>(d, CL_DEVICE_MAX_COMPUTE_UNITS)          << "\n"
                      << "\tGlobal Memory:     " << (deviceInfo<cl_ulong>(d, CL_DEVICE_GLOBAL_MEM_SIZE) >> 20)    << " MB\n"
                      // << "\tMax Clock Freq.:   " <<  deviceInfo<cl_uint>(d, CL_DEVICE_MAX_CLOCK_FREQUENCY)        << " MHz\n"
                      // << "\tMax Alloc. Memory: " << (deviceInfo<cl_ulong>(d, CL_DEVICE_MAX_MEM_ALLOC_SIZE) >> 20) << " MB\n"
                      // << "\tLocal Memory:      " << (deviceInfo<cl_ulong>(d, CL_DEVICE_LOCAL_MEM_SIZE) >> 10)     << " KB\n"
                      // << "\tAvailable:         " << (deviceInfo<cl_bool>(d, CL_DEVICE_AVAILABLE) ? "YES" : "NO")
                      << '\n';
        }
        std::cout << "\nSelect a device: " << std::flush;
        std::cin >> selected_device;
        std::cout << '\n';
    }

    return devices[selected_device];
}

//*****************************************************************
//
// OpenCL Context
//
//*****************************************************************

cl_context clCreateContextFor(cl_platform_id platform, cl_device_id device)
{
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};

    cl_int status;
    cl_context context = clCreateContext(properties, 1, &device, &clCallback, NULL, &status);
    clCheckErrorMsg(status, "Failed to create context");
    return context;
}


//*****************************************************************
//
// OpenCL Program
//
//*****************************************************************

char * loadBinaryFile(const std::string & filename, size_t * size)
{
    std::cout << "INFO: Loading " << filename << '\n';

    if (access(filename.c_str(), R_OK) != 0) {
        std::cerr << "ERROR: " << filename << " xclbin not available" << '\n';
        exit(1);
    }

    std::ifstream bin_file(filename.c_str(), std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    unsigned nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char * buf = new char[nb];
    bin_file.read(buf, nb);
    bin_file.close();

    *size = nb;

    return buf;
}


cl_program clCreateBuildProgramFromBinary(cl_context context, const cl_device_id device, const std::string & filename) {
    size_t size;
    const unsigned char * binary = reinterpret_cast<unsigned char*>(loadBinaryFile(filename, &size));
    if (binary == NULL or size == 0) {
        clCheckErrorMsg(CL_INVALID_PROGRAM, "Failed to load binary file");
    }

    cl_int status;
    cl_int binary_status;
    cl_program program = clCreateProgramWithBinary(context, 1, &device, &size, &binary, &binary_status, &status);
    clCheckErrorMsg(status, "Failed to create program with binary");
    clCheckErrorMsg(binary_status, "Failed to load binary for device");

    // It should be safe to delete the binary data
    if (binary) delete[] binary;

    status = clBuildProgram(program, 1, &device, "", NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t log_size;
        clCheckError(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size));
        std::vector<char> param_value(log_size);
        clCheckError(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, param_value.data(), NULL));
        std::cout << std::string(param_value.begin(), param_value.end()) << '\n';
        exit(-1);
    }

    return program;
}


//*****************************************************************
//
// OpenCL Event
//
//*****************************************************************

// void event_cb(cl_event event1, cl_int cmd_status, void* data) {
//     cl_int err;
//     cl_command_type command;
//     cl::Event event(event1, true);
//     OCL_CHECK(err, err = event.getInfo(CL_EVENT_COMMAND_TYPE, &command));
//     cl_int status;
//     OCL_CHECK(err, err = event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
//     const char* command_str;
//     const char* status_str;
//     switch (command) {
//         case CL_COMMAND_READ_BUFFER:
//             command_str = "buffer read";
//             break;
//         case CL_COMMAND_WRITE_BUFFER:
//             command_str = "buffer write";
//             break;
//         case CL_COMMAND_NDRANGE_KERNEL:
//             command_str = "kernel";
//             break;
//         case CL_COMMAND_MAP_BUFFER:
//             command_str = "kernel";
//             break;
//         case CL_COMMAND_COPY_BUFFER:
//             command_str = "kernel";
//             break;
//         case CL_COMMAND_MIGRATE_MEM_OBJECTS:
//             command_str = "buffer migrate";
//             break;
//         default:
//             command_str = "unknown";
//     }
//     switch (status) {
//         case CL_QUEUED:
//             status_str = "Queued";
//             break;
//         case CL_SUBMITTED:
//             status_str = "Submitted";
//             break;
//         case CL_RUNNING:
//             status_str = "Executing";
//             break;
//         case CL_COMPLETE:
//             status_str = "Completed";
//             break;
//     }
//     printf("[%s]: %s %s\n", reinterpret_cast<char*>(data), status_str, command_str);
//     fflush(stdout);
// }

// void set_callback_enqueue(cl::Event event, const char* name) {
//     cl_int err;
//     OCL_CHECK(err, err = event.setCallback(CL_QUEUED, event_cb, (void*)name));
// }

// void set_callback_complete(cl::Event event, const char* name) {
//     cl_int err;
//     OCL_CHECK(err, err = event.setCallback(CL_COMPLETE, event_cb, (void*)name));
// }

cl_ulong clTimeBetweenEventsNS(cl_event start, cl_event end)
{
    cl_ulong timeStart;
    cl_ulong timeEnd;
    clGetEventProfilingInfo(start, CL_PROFILING_COMMAND_START, sizeof(timeStart), &timeStart, NULL);
    clGetEventProfilingInfo(end, CL_PROFILING_COMMAND_END, sizeof(timeEnd), &timeEnd, NULL);

    return timeEnd - timeStart;
}

double clTimeBetweenEventsMS(cl_event start, cl_event end)
{
    return 1.e-6 * clTimeBetweenEventsNS(start, end);
}

double clTimeEventNS(cl_event event)
{
    return clTimeBetweenEventsNS(event, event);
}

double clTimeEventMS(cl_event event)
{
    return clTimeBetweenEventsMS(event, event);
}


//*****************************************************************
//
// OpenCL Context Struct
//
//*****************************************************************

struct OCL
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;

    OCL(const std::string & filename,
        int platform_id = -1,
        int device_id = -1,
        bool show_build_time = false)
    {
        platform = (platform_id < 0) ? clPromptPlatform() : clSelectPlatform(platform_id);
        device = (device_id < 0) ? clPromptDevice(platform) : clSelectDevice(platform, device_id);
        context = clCreateContextFor(platform, device);

        volatile cl_ulong time_start = current_time_ns();
        program = clCreateBuildProgramFromBinary(context, device, filename);
        volatile cl_ulong time_end = current_time_ns();

        if (show_build_time) {
            const double time_total = (time_end - time_start) * 1.0e-6;
            std::cout << "INFO: Bitstream loaded in "
                      << COUT_FLOAT << time_total << " ms"
                      << '\n';
        }
    }

    cl_command_queue createCommandQueue(bool out_of_order = false) {
        cl_int status;
        cl_command_queue queue;
        if (out_of_order) {
            queue = clCreateCommandQueue(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &status);
        } else {
            queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
        }
        clCheckErrorMsg(status, "Failed to create command queue");
        return queue;
    }

    cl_kernel createKernel(const std::string & kernel_name) {
        cl_int status;
        cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &status);
        clCheckErrorMsg(status, "Failed to create kernel");
        return kernel;
    }

    void clean() {
        if (program) clReleaseProgram(program);
        if (context) clReleaseContext(context);
    }
};

#define MAX_HBM_PC_COUNT 32
#define HBM_BANK(n) (n | XCL_MEM_TOPOLOGY)
const int hbm_bank[MAX_HBM_PC_COUNT] = {
    HBM_BANK(0),  HBM_BANK(1),  HBM_BANK(2),  HBM_BANK(3),  HBM_BANK(4),  HBM_BANK(5),  HBM_BANK(6),  HBM_BANK(7),
    HBM_BANK(8),  HBM_BANK(9),  HBM_BANK(10), HBM_BANK(11), HBM_BANK(12), HBM_BANK(13), HBM_BANK(14), HBM_BANK(15),
    HBM_BANK(16), HBM_BANK(17), HBM_BANK(18), HBM_BANK(19), HBM_BANK(20), HBM_BANK(21), HBM_BANK(22), HBM_BANK(23),
    HBM_BANK(24), HBM_BANK(25), HBM_BANK(26), HBM_BANK(27), HBM_BANK(28), HBM_BANK(29), HBM_BANK(30), HBM_BANK(31)};

}

#endif // __HOST_OCL__