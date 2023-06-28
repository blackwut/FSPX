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
    #pragma HLS INTERFACE mode=m_axi port=out bundle=out depth=SIZE num_write_outstanding=64 max_write_burst_length=64

    // std::cout << "BEFORE.eos[0] = " <<  eos[0] << std::endl;

    if (eos[0] == 0) {
        fx::StoWM(in, out, out_size, write_count, eos);
    }

    // std::cout << "AFTER.eos[0] = " <<  eos[0] << std::endl;
}
