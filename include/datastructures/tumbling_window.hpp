#ifndef __TUMBLING_WINDOW_HPP__
#define __TUMBLING_WINDOW_HPP__


namespace fx {

namespace Bucket {

//******************************************************************************
//
// Count Tumbling Window (Bucket implementation)
//
// This class implements a count tumbling window using a bucket implementation.
// This implementation is employed when the latency of the operator is greater
// than 1 and leads to a II = 1.
//
//******************************************************************************
template <typename OP, unsigned int SIZE, unsigned int LATENCY>
struct CountTumblingWindow
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using COUNT_T = ap_uint<LOG2_CEIL(SIZE)>;

    COUNT_T count;
    AGG_T aggs[L];

    CountTumblingWindow()
    : count(0)
    {
        #pragma HLS ARRAY_PARTITION variable=aggs type=complete dim=1
    }

    bool update(const IN_T in, OUT_T & out)
    {
        bool _first = (count < L);
        bool _valid = (count == COUNT_T(SIZE - 1));

        // update count
        count = _valid ? COUNT_T(0) : COUNT_T(count + 1);

        // update aggregate
        AGG_T _liftedin = OP::lift(in);
        AGG_T _tmp = _first ? OP::identity() : aggs[L - 1];
        AGG_T _agg = OP::combine(_tmp, _liftedin);

        SHIFT_LOOP:
        for (int i = L - 1; i > 0; --i) {
        #pragma HLS UNROLL
            aggs[i] = aggs[i - 1];
        }
        aggs[0] = _agg;

        // compute output (even if invalid)
        AGG_T _result = OP::identity();
        REDUCE_LOOP:
        for (int i = 0; i < MIN_VAL(SIZE, L); ++i) {
        #pragma HLS UNROLL
            _result = OP::combine(aggs[i], _result);
        }

        // output
        out = OP::lower(_result);
        return _valid;
    }
};

//******************************************************************************
//
// Count Tumbling Window (Bucket implementation)
//
// This class implements a count tumbling window using a bucket implementation.
// This implementation is employed when the latency of the operator is 1 and
// leads to a II = 1.
//
//******************************************************************************
template <typename OP, unsigned int SIZE>
struct CountTumblingWindow<OP, SIZE, 1>
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using COUNT_T = ap_uint<LOG2_CEIL(SIZE)>;

    AGG_T agg;
    COUNT_T count;

    CountTumblingWindow()
    : agg(OP::identity())
    , count(0)
    {}

    bool update(const IN_T in, OUT_T & out)
    {
        bool _first = (count == 0);
        bool _valid = (count == COUNT_T(SIZE - 1));

        // update count
        count = _valid ? COUNT_T(0) : COUNT_T(count + 1);

        // update aggregate
        AGG_T _liftedin = OP::lift(in);
        AGG_T _agg = _first ? OP::identity() : agg;
        agg = OP::combine(_agg, _liftedin);

        // output (even if invalid)
        out = OP::lower(agg);
        return _valid;
    }
};


//******************************************************************************
//
// Keyed Count Tumbling Window (Bucket implementation)
//
// This class implements a keyed count tumbling window using a bucket
// implementation. This implementation is employed when the latency of the
// operator is greater than 1 and leads to a II = LATENCY.
//
//******************************************************************************
template <typename OP, unsigned int KEYS, unsigned int SIZE>
struct KeyedCountTumblingWindow
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T = unsigned int;
    using COUNT_T = ap_uint<LOG2_CEIL(SIZE)>;

    KEY_T curr_key;
    AGG_T curr_agg;
    COUNT_T curr_count;

    AGG_T aggs[KEYS];
    COUNT_T counts[KEYS];

    KeyedCountTumblingWindow()
    : curr_key(0)
    , curr_agg(OP::identity())
    , curr_count(0)
    {
        #pragma HLS BIND_STORAGE variable=aggs   type=RAM_S2P impl=BRAM
        #pragma HLS BIND_STORAGE variable=counts type=RAM_S2P impl=BRAM
    }

    bool update(const KEY_T key, const IN_T in, OUT_T & out)
    {
        #pragma HLS DEPENDENCE variable=aggs intra RAW false
        #pragma HLS DEPENDENCE variable=aggs inter distance=2 true

        #pragma HLS DEPENDENCE variable=counts intra RAW false
        #pragma HLS DEPENDENCE variable=counts inter distance=2 true

        bool _same_key = (curr_key == key);

        // load data for current key if different from previous key
        if (!_same_key) {
            aggs[curr_key] = curr_agg;
            counts[curr_key] = curr_count;
            curr_key = key;
            curr_agg = aggs[key];
            curr_count = counts[key];
        }

        bool _first = (curr_count == 0);
        bool _valid = (curr_count == COUNT_T(SIZE - 1));

        // update count
        curr_count = _valid ? COUNT_T(0) : COUNT_T(curr_count + 1);

        // update aggregate
        AGG_T _liftedin = OP::lift(in);
        AGG_T _agg = _first ? OP::identity() : curr_agg;
        curr_agg = OP::combine(_agg, _liftedin);

        // output (even if invalid)
        out = OP::lower(curr_agg);
        return _valid;
    }
};

} // namespace Bucket
} // namespace fx

#endif // __TUMBLING_WINDOW_HPP__
