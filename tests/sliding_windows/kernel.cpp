#include "kernel.hpp"

template <typename OP, typename KEY_T>
struct Drainer
{
    void operator()(const fx::keyed_time_result_t<OP, KEY_T> in, data_t & out) {
    #pragma HLS INLINE

        out.key = in.key;
        out.value = 0;
        out.aggregate = in.value;
        out.timestamp = in.timestamp;
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using KEY_T = unsigned int;
    using OP = fx::Count<float>;
    fx::stream<fx::keyed_time_result_t<OP, KEY_T>, 64> result_stream("result_stream");

    #pragma HLS DATAFLOW

    fx::KeyedTimeSlidingWindowOperator<OP, MAX_KEYS, WINDOW_SIZE, WINDOW_STEP, WINDOW_LATENESS>(
        in, result_stream, [](const data_t & d) { return d.key; }
    );

    fx::Map<Drainer<OP, KEY_T>>(
        result_stream, out
    );
}