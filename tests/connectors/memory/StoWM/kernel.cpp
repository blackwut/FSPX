#include "kernel.hpp"

void test(
    stream_t & in,
    line_t * out,
    int out_size,
    int * write_count,
    int * eos
)
{
    #pragma HLS INTERFACE mode=m_axi port=out depth=SIZE num_write_outstanding=64 max_write_burst_length=64

    fx::StoWM(in, out, out_size, write_count, eos);
}
