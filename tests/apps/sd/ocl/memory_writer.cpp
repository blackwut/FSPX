#include "fspx.hpp"
#include "tuple.hpp"
#include "constants.hpp"


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

    #pragma HLS DATAFLOW
    fx::AStoWM(in, out, out_count, written_count, eos);
}

}