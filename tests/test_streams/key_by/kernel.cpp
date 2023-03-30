#include "kernel.hpp"

void test(
    stream_t & in,
    stream_t & out
)
{
    stream_t internal_streams[PAR];
#pragma HLS DATAFLOW
    fx::StoSN_KB<PAR>(in, internal_streams,
        [](data_t & t){
            return (t % PAR);
        });
    fx::SNtoS_KB<PAR>(internal_streams, out,
        [](int index){
        #pragma HLS INLINE
            return (index % PAR);
        });
}
