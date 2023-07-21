#include "fspx.hpp"
#include "defines.hpp"


extern "C" {
void memory_writer(
    axis_stream_t & in,
    line_t * out,
    int out_count,
    int * written_count,
    int * eos
)
{
    #pragma HLS INTERFACE mode=m_axi port=out bundle=out num_write_outstanding=64 max_write_burst_length=64

    if (eos[0] == 0) {
        fx::StoWM(in, out, out_count, written_count, eos);
    }
}

}
