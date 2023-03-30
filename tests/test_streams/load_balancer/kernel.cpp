#include "kernel.hpp"

void test(
    stream_t & in,
    stream_t & out
)
{
    stream_t internal_streams[PAR];
#pragma HLS DATAFLOW
    fx::StoSN_LB<PAR>(in, internal_streams);
    fx::SNtoS_LB<PAR>(internal_streams, out);
}
