#ifndef __SLIDING_WINDOW_HPP__
#define __SLIDING_WINDOW_HPP__

#include "tumbling_window.hpp"

namespace fx {

namespace Bucket {

//******************************************************************************
//
// Count Sliding Window (Bucket implementation)
//
// This class implements a count sliding window using a bucket implementation.
// This implementation is employed when the latency of the operator is greater
// than 1 and leads to a II = 1.
//
//******************************************************************************
template <typename OP, unsigned int SIZE, unsigned int STEP, unsigned int LATENCY>
struct CountSlidingWindow
{
    static constexpr unsigned int L = OP::LATENCY;
    static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using INDEX_T = unsigned int; // ap_uint<LOG2_CEIL(N) + 1>;
    using COUNT_T = unsigned int; // ap_uint<LOG2_CEIL(N) + 1>;

    INDEX_T bidx;
    COUNT_T count;

    CountTumblingWindow<OP, SIZE, L> buckets[N];
    bool valids[N];
    OUT_T outs[N];

    CountSlidingWindow()
    : bidx(0)
    , count(0)
    {
        #pragma HLS ARRAY_PARTITION variable=buckets type=complete dim=1
        #pragma HLS ARRAY_PARTITION variable=valids  type=complete dim=1
        #pragma HLS ARRAY_PARTITION variable=outs    type=complete dim=1
    }

    bool update(const IN_T in, OUT_T & out)
    {
        // update buckets if inside the window
        UPDATE_BUCKETS:
        for (COUNT_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            const INDEX_T idx = (i >= bidx) ? (i - bidx) : (N - bidx + i);
            if (idx * STEP <= count && count < (idx * STEP + SIZE)) {
                valids[i] = buckets[i].update(in, outs[i]);
            }
        }

        // get valid and out from first bucket (bidx)
        bool _valid = valids[bidx];
        OUT_T _out = outs[bidx];

        // update bidx (if window is complete) and count
        if (_valid) {
            bidx = (bidx == (N - 1)) ? 0 : (bidx + 1);
            count -= (STEP - 1);
        } else {
            count++;
        }

        // output (even if invalid)
        out = _out;
        return _valid;
    }
};


//******************************************************************************
//
// Keyed Count Sliding Window (Bucket implementation)
//
// This class implements a keyed count sliding window using a bucket
// implementation. This implementation is employed when the latency of the
// operator is greater than 1 and leads to a II = LATENCY.
//
//******************************************************************************
template <typename OP, unsigned int KEYS, unsigned int SIZE, unsigned int STEP>
struct KeyedCountSlidingWindow
{
    static constexpr unsigned int L = OP::LATENCY;
    static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T = unsigned int;
    using INDEX_T = unsigned int; // ap_uint<LOG2_CEIL(N) + 1>;
    using COUNT_T = unsigned int; // ap_uint<LOG2_CEIL(N) + 1>;

    KEY_T curr_key;
    INDEX_T curr_bidx;
    COUNT_T curr_count;

    INDEX_T bidxs[KEYS];
    COUNT_T counts[KEYS];

    KeyedCountTumblingWindow<OP, KEYS, SIZE> buckets[N];
    bool valid[N];
    OUT_T outs[N];

    KeyedCountSlidingWindow()
    : curr_key(0)
    , curr_bidx(0)
    , curr_count(0)
    {
        #pragma HLS ARRAY_PARTITION variable=buckets type=complete dim=1
        #pragma HLS ARRAY_PARTITION variable=valid   type=complete dim=1
        #pragma HLS ARRAY_PARTITION variable=outs    type=complete dim=1

        #pragma HLS BIND_STORAGE variable=bidxs  type=RAM_S2P impl=BRAM
        #pragma HLS BIND_STORAGE variable=counts type=RAM_S2P impl=BRAM

        INIT_BIDXS_COUNTS:
        for (int i = 0; i < KEYS; ++i) {
        #pragma HLS UNROLL
            bidxs[i] = 0;
            counts[i] = 0;
        }
    }

    bool update(KEY_T key, const IN_T in, OUT_T & out)
    {
    #pragma HLS DEPENDENCE variable=bidxs intra RAW false
    #pragma HLS DEPENDENCE variable=bidxs inter distance=2 true

    #pragma HLS DEPENDENCE variable=counts intra RAW false
    #pragma HLS DEPENDENCE variable=counts inter distance=2 true

        // load data from current key if different from previous key
        bool _same_key = (curr_key == key);
        if (!_same_key) {
            bidxs[curr_key] = curr_bidx;
            counts[curr_key] = curr_count;
            curr_key = key;
            curr_bidx = bidxs[key];
            curr_count = counts[key];
        }

        // update buckets if inside the window
        UPDATE_BUCKETS:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            INDEX_T idx = (i >= curr_bidx) ? (i - curr_bidx) : (N - curr_bidx + i);
            if (idx * STEP <= curr_count && curr_count < (idx * STEP + SIZE)) {
                valid[i] = buckets[i].update(key, in, outs[i]);
            }
        }

        // get valid and out from first bucket (base_idx)
        bool _valid = valid[curr_bidx];
        OUT_T _out = outs[curr_bidx];

        // update base_idx (if window is complete) and count
        if (_valid) {
            curr_bidx = (curr_bidx == (N - 1)) ? 0 : (curr_bidx + 1);
            curr_count -= (STEP - 1);
        } else {
            curr_count++;
        }

        // output (even if invalid)
        out = _out;
        return _valid;
    }
};


// //******************************************************************************
// //
// // Time Sliding Window (Bucket implementation)
// //
// // SIZE and STEP in nanoseconds?
// //
// //******************************************************************************
// template <typename OP, unsigned int SIZE, unsigned int STEP, unsigned int LATENCY>
// struct TimeSlidingWindow
// {
//     static constexpr unsigned int L = OP::LATENCY;
//     static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);
//     using IN_T  = typename OP::IN_T;
//     using AGG_T = typename OP::AGG_T;
//     using OUT_T = typename OP::OUT_T;

//     using INDEX_T = unsigned int;
//     using TIME_T  = unsigned int;

//     INDEX_T bidx;
//     TIME_T count;

//     CountTumblingWindow<OP, SIZE, L> buckets[N];
//     bool valids[N];
//     OUT_T outs[N];

//     TimeSlidingWindow()
//     : bidx(0)
//     , count(0)
//     {
//         #pragma HLS ARRAY_PARTITION variable=buckets type=complete dim=1
//         #pragma HLS ARRAY_PARTITION variable=valids  type=complete dim=1
//         #pragma HLS ARRAY_PARTITION variable=outs    type=complete dim=1
//     }

//     bool update(const IN_T in, OUT_T & out)
//     {
//         // update buckets if inside the window
//         UPDATE_BUCKETS:
//         for (COUNT_T i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             const INDEX_T idx = (i >= bidx) ? (i - bidx) : (N - bidx + i);
//             if (idx * STEP <= count && count < (idx * STEP + SIZE)) {
//                 valids[i] = buckets[i].update(in, outs[i]);
//             }
//         }

//         // get valid and out from first bucket (bidx)
//         bool _valid = valids[bidx];
//         OUT_T _out = outs[bidx];

//         // update bidx (if window is complete) and count
//         if (_valid) {
//             bidx = (bidx == (N - 1)) ? 0 : (bidx + 1);
//             count -= (STEP - 1);
//         } else {
//             count++;
//         }

//         // output (even if invalid)
//         out = _out;
//         return _valid;
//     }
// };

} // namespace Bucket
} // namespace fx

#endif // __SLIDING_WINDOW_HPP__
