#ifndef __FSP_SD_KERNEL_HPP__
#define __FSP_SD_KERNEL_HPP__

// #define __DEBUG__CONNECTORS__
#include "../../../include/fspx.hpp"

#include "common/utils.hpp"
#include "common/tuple.hpp"
#include "common/constants.hpp"
#include "common/moving_average.hpp"
#include "common/spike_detector.hpp"

#define SO_PAR 2
#define MA_PAR 2
#define SD_PAR 2
#define SI_PAR 1

void test(
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
);

#endif // __FSP_SD_KERNEL_HPP__
