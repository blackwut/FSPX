#ifndef __TIME_WINDOW_HPP__
#define __TIME_WINDOW_HPP__

#if !defined(__SYNTHESIS__)
#else 
#include "hls_print.h"
#endif

namespace fx {
namespace Window {


template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int LATENESS
>
struct TimeTumbling
{
    static constexpr unsigned int LATE_BUCKETS = DIV_CEIL(LATENESS, SIZE);
    static constexpr unsigned int N = 1 + LATE_BUCKETS;

    using widx_t = uint_for<N>;

    using state_t = TimeBucket_t<window_functor_t>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = TimeWindowWrapper_t<tuple_t, SIZE, SIZE>;
    using wrapper_result_t = TimeWindowResult_t<result_t, SIZE, SIZE>;
    
    timestamp_t max_timestamp;
    REMOVE_INIT(state_t state[N]);

    TimeTumbling()
    : max_timestamp(LATENESS)
    {
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=state
    
        TimeTumbling_STATE_INIT:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            state[i] = state_t();
        }
    }

    
    template <typename Shipper_t>
    void operator()(const wrapper_t & in, Shipper_t & shipper)
    {
        const tuple_t tuple = in.unwrap();
        const timestamp_t timestamp = in.get_timestamp();
        const bool flush = in.is_flush();

        const wid_t wid = in.get_last();
        const widx_t widx = widx_t(wid % N);
        const bool drop = flush || (timestamp < max_timestamp - LATENESS);
        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;

        TimeTumbling_SEND_RESULTS:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            const state_t & s = state[i];
            if (s.wid != wid_t(-1) && s.wid + LATE_BUCKETS < wid) { // TODO: check if it is correct (wid >= LATE_BUCKETS && s.wid < wid - LATE_BUCKETS)
                shipper.send(i, wrapper_result_t(0, s.state, 0, s.wid));
             }
        }

        if (!drop) {
            TimeTumbling_UPDATE_STATES:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL

                if (i == widx) {
                    state[i].update(tuple, wid);
                }
            }
        }
    }
};


template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE,
    unsigned int LATENESS
>
struct KeyedTimeTumbling
{
    static constexpr unsigned int LATE_BUCKETS = DIV_CEIL(LATENESS, SIZE);
    static constexpr unsigned int N = 1 + LATE_BUCKETS;

    using widx_t = uint_for<N>;

    using state_t = TimeBucket_t<window_functor_t>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = TimeWindowWrapper_t<tuple_t, SIZE, SIZE>;
    using wrapper_result_t = TimeWindowResult_t<result_t, SIZE, SIZE>;
    
    seq_t sequence;
    key_t key;
    timestamp_t max_timestamp;
    REMOVE_INIT(state_t state[N]);

    REMOVE_INIT(bool initialized[MAX_KEY]);
    REMOVE_INIT(timestamp_t max_timestamps[MAX_KEY]);
    REMOVE_INIT(state_t states[N][MAX_KEY]);

    KeyedTimeTumbling()
    : sequence(0)
    , key(0)
    , max_timestamp(LATENESS)
    {
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=state
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=initialized
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=max_timestamps

        #pragma HLS BIND_STORAGE type=ram_s2p impl=bram variable=states
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=states
    
        initialized[0] = true;
        KeyedTimeTumbling_INITIALIZED_INIT:
        for (unsigned int i = 1; i < MAX_KEY; ++i) {
            #pragma HLS UNROLL
            
            initialized[i] = false;
        }

        KeyedTimeTumbling_STATE_INIT:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            state[i] = state_t();
        }
    }

    template <typename Shipper_t>
    void operator()(const wrapper_t & in, Shipper_t & shipper)
    {
        #pragma HLS DEPENDENCE dependent=false direction=raw type=intra variable=states

        const key_t tuple_key = in.get_key();
        const tuple_t tuple = in.unwrap();
        const timestamp_t timestamp = in.get_timestamp();
        const bool flush = in.is_flush();

        const wid_t wid = in.get_last();
        const widx_t widx = widx_t(wid % N);

        if (key != tuple_key) {
            initialized[key]    = true;
            max_timestamps[key] = max_timestamp;
            
            KeyedTimeTumbling_STATES_STORE:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                states[i][key] = state[i];
            }

            const bool is_initialized = initialized[tuple_key];
            max_timestamp = is_initialized ? max_timestamps[tuple_key] : timestamp_t(LATENESS);

            KeyedTimeTumbling_STATES_LOAD:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                state[i] = is_initialized ? states[i][tuple_key] : state_t();
            }
        }
        key = tuple_key;

        const bool drop = flush || (timestamp < max_timestamp - LATENESS);
        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;

        KeyedTimeTumbling_SEND_RESULTS:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            const state_t & s = state[i];
            if (s.wid != wid_t(-1) && s.wid + LATE_BUCKETS < wid) { // TODO: check if it is correct (wid >= LATE_BUCKETS && s.wid < wid - LATE_BUCKETS)
                shipper.send(i, wrapper_result_t(key, s.state, sequence, s.wid));
             }
        }
        sequence++;

        if (!drop) {
            TIME_TUMBLING_UPDATE_STATES:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                if (i == widx) {
                    state[i].update(tuple, wid);
                }
            }
        }
    }
};

template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int STEP,
    unsigned int LATENESS
>
struct TimeSliding
{
    static constexpr unsigned int N = DIV_CEIL(SIZE + LATENESS, STEP);

    using widx_t = uint_for<N>;

    using state_t = TimeBucket_t<window_functor_t>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = TimeWindowWrapper_t<tuple_t, SIZE, STEP>;
    using wrapper_result_t = TimeWindowResult_t<result_t, SIZE, STEP>;

    seq_t sequence;
    key_t key;
    timestamp_t max_timestamp;
    wid_t max_wid;
    REMOVE_INIT(state_t state[N]);

    TimeSliding()
    : sequence(0)
    , max_timestamp(LATENESS)
    , max_wid(N - 1)
    {
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=state

        TimeSliding_STATE_INIT:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            state[i] = state_t();
        }
    }

    template <typename Shipper_t>
    void operator()(const wrapper_t & in, Shipper_t & shipper)
    {
        const tuple_t tuple = in.unwrap();
        const timestamp_t timestamp = in.get_timestamp();
        const bool flush = in.is_flush();

        const wid_t first_wid = in.get_first();
        const wid_t last_wid = in.get_last();

        const widx_t first_widx = widx_t(first_wid % N);
        const widx_t last_widx = widx_t(last_wid % N);

        const bool drop = flush || (timestamp < max_timestamp - LATENESS);
        max_wid = (last_wid > max_wid) ? last_wid : max_wid;
        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;
        const wid_t left_wid = max_wid - N + 1;

        TimeSliding_SEND_RESULTS:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            const state_t & s = state[i];
            if (s.wid < left_wid) {
                shipper.send(i, wrapper_result_t(sequence, s.state, s.wid, 0));
             }
        }
        sequence++;

        if (!drop) {
            TimeSliding_UPDATE_STATES:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL

                bool update = false;
                if (first_widx <= last_widx) {
                    update = (first_widx <= i && i <= last_widx);
                } else {
                    update = (i <= last_widx || first_widx <= i);
                }
                
                if (update) { // TODO: check if this is correct
                    const wid_t wid = (i >= first_widx ? first_wid + i - first_widx : first_wid + N - first_widx + i);
                    state[i].update(tuple, wid);
                    
                    // state_t & s = state[i];
                    // result_t tmp = (s.wid == wid_t(-1) || s.wid < left_wid) ? result_t() : s.state;
                    
                    // static window_functor_t window_functor;
                    // window_functor(tuple, tmp);

                    // s.wid = (i >= first_widx ? first_wid + i - first_widx : first_wid + N - first_widx + i);
                    // s.state = tmp;
                }
            }
        }
    }
};

template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE,
    unsigned int STEP,
    unsigned int LATENESS
>
struct KeyedTimeSliding
{
    static constexpr unsigned int N = DIV_CEIL(SIZE + LATENESS, STEP);

    using widx_t = uint_for<N>;

    using state_t = TimeBucket_t<window_functor_t>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = TimeWindowWrapper_t<tuple_t, SIZE, STEP>;
    using wrapper_result_t = TimeWindowResult_t<result_t, SIZE, STEP>;

    seq_t sequence;
    key_t key;
    timestamp_t max_timestamp;
    wid_t max_wid;
    REMOVE_INIT(state_t state[N]);

    REMOVE_INIT(bool initialized[MAX_KEY]);
    REMOVE_INIT(timestamp_t max_timestamps[MAX_KEY]);
    REMOVE_INIT(wid_t max_wids[MAX_KEY]);
    REMOVE_INIT(state_t states[N][MAX_KEY]);

    KeyedTimeSliding()
    : sequence(0)
    , key(0)
    , max_timestamp(LATENESS)
    , max_wid(N - 1)
    {
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=state
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=initialized
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=max_timestamps
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=max_wids

        #pragma HLS BIND_STORAGE type=ram_s2p impl=bram variable=states
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=states

        initialized[0] = true;
        KeyedTimeSliding_INITIALIZED_INIT:
        for (unsigned int i = 1; i < MAX_KEY; ++i) {
        #pragma HLS UNROLL
            initialized[i] = false;
        }

        KeyedTimeSliding_STATE_INIT:
        for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            state[i] = state_t();
        }
    }

    template <typename Shipper_t>
    void operator()(const wrapper_t & in, Shipper_t & shipper)
    {
        #pragma HLS DEPENDENCE dependent=false direction=raw type=intra variable=states

        const key_t tuple_key = in.get_key();
        const tuple_t tuple = in.unwrap();
        const timestamp_t timestamp = in.get_timestamp();
        const bool flush = in.is_flush();

        const wid_t first_wid = in.get_first();
        const wid_t last_wid = in.get_last();

        const widx_t first_widx = widx_t(first_wid % N);
        const widx_t last_widx = widx_t(last_wid % N);

        if (key != tuple_key) {
            initialized[key]    = true;
            max_timestamps[key] = max_timestamp;
            max_wids[key]       = max_wid;

            KeyedTimeSliding_STATES_STORE:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                states[i][key] = state[i];
            }

            const bool is_initialized = initialized[tuple_key];
            max_timestamp = is_initialized ? max_timestamps[tuple_key] : timestamp_t(LATENESS);
            max_wid       = is_initialized ? max_wids[tuple_key]       : wid_t(N - 1);

            KeyedTimeSliding_STATES_LOAD:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                state[i] = is_initialized ? states[i][tuple_key] : state_t();
            }
        }
        key = tuple_key;

        const bool drop = flush || (timestamp < max_timestamp - LATENESS);
        max_wid = (last_wid > max_wid) ? last_wid : max_wid;
        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;
        const wid_t left_wid = max_wid - N + 1;

        KeyedTimeSliding_SEND_RESULTS:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL

            const state_t & s = state[i];
            if (s.wid < left_wid) {
                shipper.send(i, wrapper_result_t(sequence, s.state, s.wid, tuple_key));
             }
        }
        sequence++;

        if (!drop) {
            KeyedTimeSliding_UPDATE_STATES:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL

                bool update = false;
                if (first_widx <= last_widx) {
                    update = (first_widx <= i && i <= last_widx);
                } else {
                    update = (i <= last_widx || first_widx <= i);
                }
                
                if (update) { // TODO: check if this is correct
                    const wid_t wid = (i >= first_widx ? first_wid + i - first_widx : first_wid + N - first_widx + i);
                    state[i].update(tuple, wid);

                    // state_t & s = state[i];
                    // result_t tmp = (s.wid == wid_t(-1) || s.wid < left_wid) ? result_t() : s.state;
                    
                    // static window_functor_t window_functor;
                    // window_functor(tuple, tmp);

                    // s.wid = (i >= first_widx ? first_wid + i - first_widx : first_wid + N - first_widx + i);
                    // s.state = tmp;
                }
            }
        }
    }
};

} // namespace Window
} // namespace fx


#endif // __TIME_WINDOW_HPP__