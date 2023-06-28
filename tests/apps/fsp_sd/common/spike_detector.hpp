#ifndef __SPIKE_DETECTOR_HPP__
#define __SPIKE_DETECTOR_HPP__

#ifdef __SYNTHESIS__
#include <hls_math.h>
#endif

#include "tuple.hpp"

template <typename float_t>
struct SpikeDetector
{
    static constexpr float_t THRESHOLD = float_t(0.025);

    void operator() (record_t in, record_t & out, bool & result) {
    #pragma HLS INLINE
        out.key                 = in.key;
        out.property_value      = in.property_value;
        out.incremental_average = in.incremental_average;
        out.timestamp           = in.timestamp;
        result = (fabs(in.property_value - in.incremental_average) > (THRESHOLD * in.incremental_average));
    }
};

#endif // __SPIKE_DETECTOR_HPP__