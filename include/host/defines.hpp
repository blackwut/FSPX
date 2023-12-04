#ifndef __HOST_DEFINES_HPP__
#define __HOST_DEFINES_HPP__

#ifdef WF_RELEASE
#define COMMAND_QUEUE_PROFILE                   false
#define STREAM_GENERATOR_PRINT_TRANSFER_INFO    0
#define STREAM_DRAINER_PRINT_TRANSFER_INFO      0
#else
#define COMMAND_QUEUE_PROFILE                   true
#define STREAM_GENERATOR_PRINT_TRANSFER_INFO    1
#define STREAM_DRAINER_PRINT_TRANSFER_INFO      1
#endif

#ifdef WF_HOSTMEM
#define STREAM_GENERATOR_USE_HOSTMEM            true
#define STREAM_DRAINER_USE_HOSTMEM              true
#else
#define STREAM_GENERATOR_USE_HOSTMEM            false
#define STREAM_DRAINER_USE_HOSTMEM              false
#endif

#endif // __HOST_DEFINES_HPP__