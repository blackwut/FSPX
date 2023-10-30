#include "kernel.hpp"

template <typename T, typename OPERATOR>
struct window_flatmap
{
    using OUT_T = typename OPERATOR::OUT_T;
    fx::TumblingCountWindow<OPERATOR, WINDOW_STEP> window;

    template <typename U>
    void operator()(T & in, fx::FlatMapShipper<U> & shipper)
    {
    #pragma HLS INLINE
        bool valid = false;
        OUT_T agg;
        window.update(in.value, agg, valid);

        if (valid) {
            T out;
            out.key = in.key;
            out.value = in.value;
            out.aggregate = agg;
            shipper.send(out);
        }
    }
};

template <typename T, typename OPERATOR>
struct keyed_window_flatmap
{
    using OUT_T = typename OPERATOR::OUT_T;
    fx::KeyedTumblingCountWindow<OPERATOR, WINDOW_STEP, MAX_KEYS> window;

    template <typename U>
    void operator()(data_t & in, fx::FlatMapShipper<U> & shipper)
    {
    #pragma HLS INLINE
        bool valid = false;
        OUT_T agg;
        window.update(in.key, in.value, agg, valid);

        if (valid) {
            data_t out;
            out.key = in.key;
            out.value = in.value;
            out.aggregate = agg;
            shipper.send(out);
        }
    }
};


void test(
    in_stream_t & in,
    out_stream_t & out
)
{
    #pragma HLS DATAFLOW

    fx::FlatMap<keyed_window_flatmap<float, fx::Count<float> > >(in, out);
}
