#include "kernel.hpp"

using stream_t = fx::stream<record_t, 8>;

void test(
    axis_stream_t & in,
    axis_stream_t & out
)
{
    #pragma HLS DATAFLOW
    static MovingAverage<float> ma{};
    stream_t internal_stream;
    static SpikeDetector sd{};

    fx::Map(in, internal_stream, ma);
    fx::Filter(internal_stream, out, sd);
}
