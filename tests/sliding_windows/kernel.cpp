#include "kernel.hpp"

template <typename T, typename OPERATOR>
struct window_flatmap
{
    using OUT_T = typename OPERATOR::OUT_T;
    fx::SlidingCountWindow<OPERATOR, WINDOW_SIZE, WINDOW_STEP> window;

    template <typename U>
    void operator()(T & in, fx::FlatMapShipper<U> & shipper)
    {
    #pragma HLS INLINE
        bool valid = false;
        OUT_T aggr;
        window.update(in.value, aggr, valid);

        if (valid) {
            T out;
            out.key = in.key;
            out.value = in.value;
            out.aggregate = aggr;
            shipper.send(out);
        }
    }
};

template <typename T, typename OPERATOR>
struct keyed_window_flatmap
{
    using OUT_T = typename OPERATOR::OUT_T;
    fx::KeyedSlidingCountWindow<OPERATOR, WINDOW_SIZE, WINDOW_STEP, MAX_KEYS> window;

    template <typename U>
    void operator()(T & in, fx::FlatMapShipper<U> & shipper)
    {
    #pragma HLS INLINE
        bool valid = false;
        OUT_T aggr;
        window.update(in.key, in.value, aggr, valid);

        if (valid) {
            T out;
            out.key = in.key;
            out.value = in.value;
            out.aggregate = aggr;
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

    fx::FlatMap<window_flatmap<data_t, fx::Sum<float> > >(in, out);
    // fx::FlatMap<keyed_window_flatmap<data_t, fx::Count<float> > >(in, out);
}
