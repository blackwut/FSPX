#ifndef __TUMBLING_WINDOW_HPP__
#define __TUMBLING_WINDOW_HPP__

#include "aggregate_operators.hpp"

namespace fx {

template <
    typename OPERATOR,  // functor to combine two elements
    unsigned int SIZE   // window size
>
struct TumblingCountWindow
{
    using IN_T  = typename OPERATOR::IN_T;
    using AGG_T = typename OPERATOR::AGG_T;
    using OUT_T = typename OPERATOR::OUT_T;

    AGG_T agg;
    unsigned int count;

    TumblingCountWindow()
    : agg(OPERATOR::identity())
    , count(0)
    {}

    void update(const IN_T & in, OUT_T & out, bool & valid) {
    #pragma HLS INLINE
        if (count < (SIZE - 1)) {
            out = OPERATOR::lower(OPERATOR::identity());
            valid = false;
            agg = OPERATOR::combine(agg, OPERATOR::lift(in));
            count++;
        } else {
            out = OPERATOR::lower(OPERATOR::combine(agg, OPERATOR::lift(in)));
            valid = true;
            agg = OPERATOR::identity();
            count = 0;
        }
    }
};

template <
    typename OPERATOR,  // functor to combine two elements
    unsigned int SIZE,  // window size
    unsigned int KEYS   // max number of keys
>
struct KeyedTumblingCountWindow
{
    using IN_T  = typename OPERATOR::IN_T;
    using OUT_T = typename OPERATOR::OUT_T;

    TumblingCountWindow<OPERATOR, SIZE> windows[KEYS];

    KeyedTumblingCountWindow()
    {
        #pragma HLS ARRAY_PARTITION variable = windows complete dim = 1
    }

    void update(const unsigned int & key, const IN_T & in, OUT_T & out, bool & valid) {
    #pragma HLS INLINE
        windows[key].update(in, out, valid);
    }
};

}

#endif // __TUMBLING_WINDOW_HPP__