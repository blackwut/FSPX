#include "kernel.hpp"

void test(
    line_t * in,
    stream_t outs[READ_ITEMS],
    int in_count
)
{
    #pragma HLS interface mode=m_axi port=in bundle=in depth=SIZE num_read_outstanding=64 max_read_burst_length=64
    fx::WMtoSN_RR(in, in_count, true, outs);
}
