#include "kernel.hpp"

using stream_t = fx::stream<record_t, 8>;

void test(
    axis_stream_t & in,
    line_t * out,
    int out_size,
    int * write_count,
    int * eos
)
{
    #pragma HLS INTERFACE mode=m_axi port=out depth=SIZE num_write_outstanding=64 max_write_burst_length=64

    #pragma HLS DATAFLOW
    stream_t internal_stream;

    fx::AtoS(in, internal_stream);
    fx::StoWM(internal_stream, out, out_size, write_count, eos);
}