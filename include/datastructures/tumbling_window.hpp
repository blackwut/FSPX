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

    COUNT_T counts[KEYS];
    AGG_T aggs[KEYS];

    KeyedCountTumblingWindow()
    : curr_key(0)
    , curr_agg(OP::identity())
    , curr_count(0)
    {
        // RAM_S2P: A dual-port RAM that allows read operations on one port
        //          and write operations on the other port.
        #pragma HLS BIND_STORAGE variable=counts type=RAM_S2P impl=BRAM
        #pragma HLS BIND_STORAGE variable=aggs   type=RAM_S2P impl=BRAM
    }

    bool update(const KEY_T key, const IN_T in, OUT_T & out)
    {
        #pragma HLS DEPENDENCE variable=counts    intra RAW false
        // #pragma HLS DEPENDENCE variable=counts inter     false
        #pragma HLS DEPENDENCE variable=aggs      intra RAW false
        // #pragma HLS DEPENDENCE variable=aggs   inter     false

        bool _same_key = (curr_key == key);

        // load data for current key if different from previous key
        if (!_same_key) {
            counts[curr_key] = curr_count;
            aggs[curr_key] = curr_agg;

            curr_key = key;
            curr_count = counts[key];
            curr_agg = aggs[key];
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


//******************************************************************************
//
// @brief Time Tumbling Window (Bucket implementation)
//
// @details
// This class implements a count tumbling window using a bucket implementation.
// This implementation is employed when the latency of the operator is greater
// than 1 and leads to a II = 1.
//
// @tparam OP The aggregate operator to be applied to the window
// @tparam SIZE The size of the window in microseconds
//
//******************************************************************************
template <typename OP, unsigned int SIZE, unsigned int LATENCY>
struct TimeTumblingWindow
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using COUNT_T = unsigned int;
    using TIME_T  = unsigned int;
    using WIN_T   = unsigned int;

    COUNT_T count;
    TIME_T offset;
    TIME_T window_id;

    AGG_T aggs[L];

    TimeTumblingWindow(const TIME_T offset = 0)
    : count(0)
    , offset(offset)
    , window_id(0)
    {
        #pragma HLS ARRAY_PARTITION variable=aggs type=complete dim=1
    }

    //
    // @brief Update the window with a new input
    //
    // @details
    // This method updates the window with a new input value. The output is
    // valid if the window is closed on this update.
    // The window trigger when the timestamp of the input value is greater than
    // the end of the window. This means that, to trigger the last window, this
    // method must be called another time after the last input with a timestamp
    // greater than the end of the window, e.g. with a timestamp equal to
    // std::numeric_limits<unsigned int>::max().
    //
    // @param in The input value
    // @param timestamp The timestamp of the input value
    // @param out The output value
    //
    // @return True if the output is valid, false otherwise
    //
    bool update(const IN_T in, const TIME_T timestamp, OUT_T & out)
    {
        bool _first = (count < L);

        TIME_T time_end = offset + window_id * SIZE + TIME_T(SIZE - 1);
        bool _valid = timestamp > time_end;
        
        if (_valid) {
            AGG_T _result = OP::identity();
            REDUCE_LOOP:
            for (int i = 0; i < L; ++i) {
            #pragma HLS UNROLL
                _result = OP::combine((i < count ? aggs[i] : OP::identity()), _result);
            }
            
            window_id = (timestamp - offset) / SIZE;
            out = OP::lower(_result);
            count = 1;
        } else {
            out = OP::lower(OP::identity());
            count++;
        }

        AGG_T _liftedin = OP::lift(in);
        AGG_T _tmp = _first | _valid ? OP::identity() : aggs[L - 1];
        AGG_T _agg = OP::combine(_tmp, _liftedin);

        SHIFT_LOOP:
        for (int i = L - 1; i > 0; --i) {
        #pragma HLS UNROLL
            aggs[i] = aggs[i - 1];
        }
        aggs[0] = _agg;

        return _valid;
    }
};

//******************************************************************************
//
// @brief Time Tumbling Window (Bucket implementation)
//
// @details
// This class implements a count tumbling window using a bucket implementation.
// This implementation is employed when the latency of the operator is 1 and
// leads to a II = 1.
//
// @tparam OP The aggregate operator to be applied to the window
// @tparam SIZE The size of the window in microseconds
//
//******************************************************************************
#if 0
template <typename OP, unsigned int SIZE>
struct TimeTumblingWindow<OP, SIZE, 1>
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    TIME_T offset;
    WIN_T window_id;

    AGG_T agg;

    TimeTumblingWindow(const TIME_T offset = 0)
    : offset(offset)
    , window_id(0)
    , agg(OP::identity())
    {}

    //
    // @brief Update the window with a new input
    //
    // @details
    // This method updates the window with a new input value. The output is
    // valid if the window is closed on this update.
    // The window trigger when the timestamp of the input value is greater than
    // the end of the window. This means that, to trigger the last window, this
    // method must be called another time after the last input with a timestamp
    // greater than the end of the window, e.g. with a timestamp equal to
    // std::numeric_limits<unsigned int>::max().
    //
    // @param in The input value
    // @param timestamp The timestamp of the input value
    // @param out The output value
    //
    // @return True if the output is valid, false otherwise
    //
    bool update(const IN_T in, const TIME_T timestamp, OUT_T & out)
    {
        TIME_T time_end = offset + window_id * SIZE + TIME_T(SIZE - 1);
        bool _valid = timestamp > time_end;
        if (_valid) {
            out = OP::lower(agg);
            window_id = (timestamp - offset) / SIZE;
            // time_end =  offset + (((timestamp - offset) / SIZE) * SIZE) + TIME_T(SIZE - 1);
        } else {
            out = OP::lower(OP::identity());
        }

        // update aggregate
        AGG_T _liftedin = OP::lift(in);
        AGG_T _agg = _valid ? OP::identity() : agg;
        agg = OP::combine(_agg, _liftedin);

        return _valid;
    }
};
#else
#include "bucket.hpp"

template <typename OP, unsigned int SIZE>
struct TimeTumblingWindow<OP, SIZE, 1>
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    time_bucket_t<OP, SIZE> bucket;

    TimeTumblingWindow(const TIME_T offset = 0)
    : bucket(offset)
    {}

    bool update(const IN_T in, const TIME_T timestamp, OUT_T & out)
    {
        typename time_bucket_t<OP, SIZE>::result_t result;
        bucket.insert(in, timestamp, result);
        out = result.value;
        return result.valid;
    }
};
#endif

//******************************************************************************
//
// Keyed Time Tumbling Window (Bucket implementation)
//
//
//******************************************************************************
#if 0
template <typename OP, unsigned int KEYS, unsigned int SIZE>
struct KeyedTimeTumblingWindow
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T  = unsigned int;
    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    TIME_T offset;
    KEY_T curr_key;
    AGG_T curr_agg;
    WIN_T curr_window_id;

    TIME_T window_ids[KEYS];
    AGG_T aggs[KEYS];

    KeyedTimeTumblingWindow(const TIME_T offset = 0)
    : offset(offset)
    , curr_key(0)
    , curr_agg(OP::identity())
    , curr_window_id(0)
    {
        // RAM_S2P: A dual-port RAM that allows read operations on one port
        //          and write operations on the other port.
        #pragma HLS BIND_STORAGE variable=window_ids type=RAM_S2P impl=BRAM
        #pragma HLS BIND_STORAGE variable=aggs       type=RAM_S2P impl=BRAM
    }

    //
    // @brief Update the window with a new input
    //
    // @details
    // This method updates the window with a new input value. The output is
    // valid if the window is closed on this update.
    // The window trigger when the timestamp of the input value is greater than
    // the end of the window. This means that, to trigger the last window, this
    // method must be called another time after the last input with a timestamp
    // greater than the end of the window, e.g. with a timestamp equal to
    // std::numeric_limits<unsigned int>::max().
    //
    // @param key The key of the input value
    // @param in The input value
    // @param timestamp The timestamp of the input value
    // @param out The output value
    //
    // @return True if the output is valid, false otherwise
    //
    bool update(const KEY_T key, const IN_T in, const TIME_T timestamp, OUT_T & out)
    {
        #pragma HLS DEPENDENCE variable=window_ids    intra RAW false
        // #pragma HLS DEPENDENCE variable=window_ids inter     false
        #pragma HLS DEPENDENCE variable=aggs          intra RAW false
        // #pragma HLS DEPENDENCE variable=aggs       inter     false
        

        bool _same_key = (curr_key == key);

        // load data for current key if different from previous key
        if (!_same_key) {
            window_ids[curr_key] = curr_window_id;
            aggs[curr_key] = curr_agg;

            curr_key = key;
            curr_window_id = window_ids[key];
            curr_agg = aggs[key];
        }

        TIME_T time_end = offset + curr_window_id * SIZE + TIME_T(SIZE - 1);
        bool _valid = timestamp > time_end;

        if (_valid) {
            out = OP::lower(curr_agg);
            curr_window_id = (timestamp - offset) / SIZE;
        } else {
            out = OP::lower(OP::identity());
        }

        // update aggregate
        AGG_T _liftedin = OP::lift(in);
        AGG_T _agg = _valid ? OP::identity() : curr_agg;
        curr_agg = OP::combine(_agg, _liftedin);

        return _valid;
    }
};
#else
#include "bucket.hpp"
template <typename OP, unsigned int KEYS, unsigned int SIZE>
struct KeyedTimeTumblingWindow
{
    using BUCKET_T = time_bucket_t<OP, SIZE>;
    using STATE_T = typename BUCKET_T::state_t;

    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T  = unsigned int;
    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    TIME_T offset;
    KEY_T curr_key;
    
    STATE_T states[KEYS];
    BUCKET_T bucket;

    KeyedTimeTumblingWindow(const TIME_T offset = 0)
    : offset(offset)
    , curr_key(0)
    {
        #pragma HLS BIND_STORAGE variable=states type=RAM_S2P impl=BRAM
    }

    bool update(const KEY_T key, const IN_T in, const TIME_T timestamp, OUT_T & out)
    {
        #pragma HLS DEPENDENCE variable=states intra RAW false
        
        bool _same_key = (curr_key == key);

        // load data for current key if different from previous key
        if (!_same_key) {
            states[curr_key] = bucket.get_state();
            curr_key = key;
            bucket.set_state(states[key]);
        }

        typename BUCKET_T::result_t result;
        bucket.insert(in, timestamp, result);
        out = result.value;
        return result.valid;
    }
};
#endif


#include "bucket.hpp"

template <typename OP, unsigned int SIZE, unsigned int LATENESS, typename STREAM_OUT>
struct LateTimeTumblingWindow
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;


    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    late_time_bucket_t<OP, SIZE, LATENESS, STREAM_OUT> bucket;

    static constexpr unsigned int FLUSH_SIZE = late_time_bucket_t<OP, SIZE, LATENESS, STREAM_OUT>::N;

    STREAM_OUT & shipper;

    LateTimeTumblingWindow(STREAM_OUT & shipper)
    : shipper(shipper)
    , bucket(shipper)
    {}

    void process(const IN_T in, const TIME_T timestamp, const bool valid)
    {
    #pragma HLS INLINE
        bucket.process(in, timestamp, valid);
    }

    void flush()
    {
    // #pragma HLS INLINE
    //     FLUSH_BUCKETS:
    //     for (unsigned int i = 0; i < FLUSH_SIZE; i++) {
    //         #pragma HLS UNROLL
    //         bucket.process(OP::lower(OP::identity()), -1, false);
    //     }

        // TODO: find a way to flush the bucket with the new implementation!
        // bucket.process(OP::lower(OP::identity()), -1, true);
        
        // LATE_TIME_TUMBLING_WINDOW_FLUSH:
        // for (unsigned int i = 0; i < FLUSH_SIZE - 1; i++) {
        // #pragma HLS UNROLL
        //     std::cout << "FLUSHING: result " << i << std::endl;
        //     bucket.process(OP::lower(OP::identity()), 0, false);    
        // }
    }
};

} // namespace Bucket
} // namespace fx

#endif // __TUMBLING_WINDOW_HPP__
