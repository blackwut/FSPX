#include "fspx.hpp"
#include "tuple.hpp"
#include "constants.hpp"

using stream_t = fx::stream<record_t, 8>;

extern "C" {
    void memory_reader(
        line_t * in,
        int in_count,
        int eos,
        axis_stream_t & out
    )
    {
        #pragma HLS INTERFACE mode=m_axi port=in bundle=in num_read_outstanding=64 max_read_burst_length=64

        fx::WMtoS(in, in_count, (eos != 0), out);
    }
}