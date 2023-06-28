#include "kernel.hpp"

void test(
    stream_t in[IN_PAR],
    stream_t out[OUT_PAR]
)
{
    #pragma HLS dataflow
    stream_t internal_streams[IN_PAR][OUT_PAR];

    auto key_extractor = [](const data_t & t) -> int {
    #pragma HLS INLINE
        return (int)(t.key % OUT_PAR);
    };
    // auto key_generator = [](const int i) -> int {
    // #pragma HLS INLINE
    //     switch (i % 4) {
    //         case 0: return 1;
    //         case 1: return 0;
    //         case 2: return 3;
    //         case 3: return 2;
    //     }
    //     return 0;
    // };

    fx::A2A::Emitter_KB<IN_PAR, OUT_PAR>(in, internal_streams, key_extractor);
    fx::A2A::Collector_RR<IN_PAR, OUT_PAR>(internal_streams, out);
}
