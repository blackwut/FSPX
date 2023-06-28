#include "kernel.hpp"

using stream_t = fx::stream<record_t, 8>;

void test(
    line_t * in,
    axis_stream_t & out,
    int in_count
)
{
    #pragma HLS interface mode=m_axi port=in bundle=in depth=SIZE num_read_outstanding=64 max_read_burst_length=64

    #pragma HLS DATAFLOW
    stream_t internal_stream;

    fx::WMtoS(in, in_count, true, internal_stream);
    fx::StoA(internal_stream, out);
}
