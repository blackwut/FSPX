#include "kernel.hpp"

void test(
    line_t * in,
    int in_count,
    int eos,
    axis_stream_t & out
)
{
    #pragma HLS interface mode=m_axi port=in bundle=in depth=SIZE num_read_outstanding=64 max_read_burst_length=64

    fx::WMtoS(in, in_count, (eos != 0), out);
}
