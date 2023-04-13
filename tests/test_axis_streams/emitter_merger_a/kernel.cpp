#include "kernel.hpp"

void test(
    axis_stream_t & in,
    axis_stream_t & out
)
{
    stream_t internal_streams[PAR];

    #pragma HLS DATAFLOW
    fx::AtoSN_RR<PAR>(in, internal_streams);
    fx::SNtoA_RR<PAR>(internal_streams, out);
}
