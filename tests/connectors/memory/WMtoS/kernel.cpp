#include "kernel.hpp"

void test(
    line_t * in,
    stream_t & out,
    int in_count
)
{
    #pragma HLS interface mode=m_axi port=in bundle=in depth=SIZE num_read_outstanding=64 max_read_burst_length=64
    fx::WMtoS(in, in_count, true, out);
}
