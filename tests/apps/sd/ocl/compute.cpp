#include "fspx.hpp"

#include "tuple.hpp"
#include "constants.hpp"
#include "moving_average.hpp"
#include "spike_detector.hpp"

using stream_t = fx::stream<record_t, 8>;

extern "C" {
void compute(
    axis_stream_t & in,
    axis_stream_t & out
)
{
    #pragma HLS interface ap_ctrl_none port=return

    #pragma HLS DATAFLOW
    static MovingAverage<float> ma{};
    stream_t internal_stream;
    static SpikeDetector sd{};

    fx::Map(in, internal_stream, ma);
    fx::Filter(internal_stream, out, sd);
}
}
