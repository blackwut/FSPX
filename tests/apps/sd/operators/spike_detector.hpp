#ifndef __SPIKE_DETECTOR_HPP__
#define __SPIKE_DETECTOR_HPP__

#ifdef __SYNTHESIS__
#include <hls_math.h>
#endif

#include "common/constants.hpp"
#include "tuples/record_t.hpp"

template <typename float_t>
struct SpikeDetector
{
    void operator() (record_t in, record_t & out, bool & result) {
    #pragma HLS INLINE
        out.key                 = in.key;
        out.property_value      = in.property_value;
        out.incremental_average = in.incremental_average;
        out.timestamp           = in.timestamp;
        result = (fabs(in.property_value - in.incremental_average) > (THRESHOLD<float_t> * in.incremental_average));
    }
};

#endif // __SPIKE_DETECTOR_HPP__