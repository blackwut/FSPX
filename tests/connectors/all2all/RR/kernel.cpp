#include "kernel.hpp"

void test(
    stream_t in[IN_PAR],
    stream_t out[OUT_PAR]
)
{
    #pragma HLS dataflow
    stream_t internal_streams[IN_PAR][OUT_PAR];

    fx::A2A::Emitter_RR<IN_PAR, OUT_PAR>(in, internal_streams);
    fx::A2A::Collector_RR<IN_PAR, OUT_PAR>(internal_streams, out);
}
