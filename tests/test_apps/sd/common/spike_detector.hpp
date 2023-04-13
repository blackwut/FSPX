#ifndef __SPIKE_DETECTOR_HPP__
#define __SPIKE_DETECTOR_HPP__

#ifdef __SYNTHESIS__
#include <hls_math.h>
#endif

#include "tuple.hpp"

struct SpikeDetector
{
    static constexpr float THRESHOLD = 0.025f;

    bool operator() (record_t in, record_t & out) {
    #pragma HLS INLINE
        out.key                 = in.key;
        out.property_value      = in.property_value;
        out.incremental_average = in.incremental_average;
        out.timestamp           = in.timestamp;
        return (fabsf(in.property_value - in.incremental_average) > (THRESHOLD * in.incremental_average));
    }
};

#endif // __SPIKE_DETECTOR_HPP__