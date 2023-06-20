#include "kernel.hpp"

void test(
    stream_t & in,
    stream_t & out
)
{
    stream_t internal_streams[PAR];
#pragma HLS DATAFLOW
    // y[1] of data_t contains the index of the tuple.
    // (y[1] + 2) % PAR results in a RoundRobin dispatch shifted by 2
    // i.e. 3 4 1 2
    fx::StoSN_KB<PAR>(in, internal_streams,
        [](data_t & t){
            return (t.y_[1] + 2) % PAR;
        });

    // 4 3 2 1
    fx::SNtoS_KB<PAR>(internal_streams, out,
        [](int i){
        #pragma HLS INLINE
            switch (i % 4) {
                case 0: return 1;
                case 1: return 0;
                case 2: return 3;
                case 3: return 2;
            }
            return 0;
        });
}
