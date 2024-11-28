#include "kernel.hpp"

void kernel(in_stream_t & in, out_stream_t & out)
{
    using window_result_t = fx::CountWindowResult_t<result_t, WINDOW_SIZE>;
    fx::stream<window_result_t, 64> result_stream("result_stream");

    #pragma HLS DATAFLOW

    fx::CountTumblingWindow<window_functor, WINDOW_SIZE>(
        in, result_stream
    );

    fx::Map<Drainer>(
        result_stream, out
    );
}