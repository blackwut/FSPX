#include "fspx.hpp"
#include "tuple.hpp"
#include "constants.hpp"


using stream_t = fx::stream<record_t, 8>;

extern "C" {
void memory_writer(
    axis_stream_t & in,
    line_t * out,
    int out_count,
    int * written_count,
    int * eos
)
{
    #pragma HLS INTERFACE mode=m_axi port=out num_write_outstanding=64 max_write_burst_length=64

    #pragma HLS DATAFLOW
    stream_t internal_stream;

    fx::AtoS(in, internal_stream);
    fx::StoWM(internal_stream, out, out_count, written_count, eos);
}
}

