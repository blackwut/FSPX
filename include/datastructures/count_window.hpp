#ifndef __COUNT_WINDOW_HPP__
#define __COUNT_WINDOW_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"
#include "window_common.hpp"

namespace fx {
namespace Window {


//******************************************************************************
//
// Count Tumbling Window
//
//******************************************************************************
template <
    typename window_functor_t,
    unsigned int SIZE
>
struct CountTumbling
{
    using state_t = CountBucket_t<window_functor_t, SIZE>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = CountWindowWrapper_t<tuple_t, SIZE>;
    using wrapper_result_t = CountWindowResult_t<result_t, SIZE>;

    state_t state;

    CountTumbling()
    : state()
    {}

    void operator()(const wrapper_t & in, wrapper_result_t & out, bool & valid)
    {
        const tuple_t & tuple = in.unwrap();
        const bool flush = in.is_flush();

        valid = state.update(tuple, flush);
        out = wrapper_result_t(0, state.state, state.wid, state.count);
    }
};


//******************************************************************************
//
// Keyed Count Tumbling Window NOT WORKING
//
//******************************************************************************
template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE
>
struct KeyedCountTumbling
{
    using count_t = uint_for<SIZE>;

    using state_t = CountBucket_t<window_functor_t, SIZE>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = CountWindowWrapper_t<tuple_t, SIZE>;
    using wrapper_result_t = CountWindowResult_t<result_t, SIZE>;

    key_t key;
    state_t state;

    REMOVE_INIT(bool initialized[MAX_KEY]);
    REMOVE_INIT(state_t states[MAX_KEY]);

    KeyedCountTumbling()
    : key(0)
    , state()
    {
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=initialized
        #pragma HLS BIND_STORAGE type=ram_s2p impl=bram variable=states
    
        initialized[0] = true;
        KeyedCountTumbling_INIT:
        for (unsigned int i = 1; i < MAX_KEY; ++i) {
            #pragma HLS UNROLL
            
            initialized[i] = false;
        }
    }

    void operator()(const wrapper_t & in, wrapper_result_t & out, bool & valid)
    {
        #pragma HLS DEPENDENCE dependent=false direction=raw type=intra variable=states

        const key_t tuple_key = in.get_key();
        const tuple_t tuple = in.unwrap();
        const bool flush = in.is_flush();

        if (key != tuple_key) {
            initialized[key] = true;
            states[key] = state;
            
            state = initialized[tuple_key] ? states[tuple_key] : state_t();
        }
        key = tuple_key;

        valid = state.update(tuple, flush);
        out = wrapper_result_t(key, state.state, state.wid, state.count);
    }
};


template <
    unsigned int SIZE,
    unsigned int STEP
>
struct window_index_t // TODO: use uint_for and increment() function from common.hpp
{
    static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);
    using count_t = ap_uint<LOG2_CEIL(STEP)>;
    using idx_t = ap_uint<LOG2_CEIL(N + (N == 1))>;

    count_t count;
    idx_t widx;

    window_index_t()
    : count(count_t(STEP - 1))
    , widx(idx_t(0))
    {}

    window_index_t & operator=(const window_index_t & other)
    {
        count = other.count;
        widx = other.widx;
        return *this;
    }

    void increment()
    {
        #pragma HLS INLINE
        
        if (count == count_t(STEP - 1)) {
            count = 0;
            widx = (widx == idx_t(N - 1)) ? idx_t(0) : idx_t(widx + 1);
        } else {
            count++;
        }
    }

    idx_t get() const
    {
        #pragma HLS INLINE
        return widx;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const window_index_t & widx)
    {
        os << "("
           << "count: " << std::setw(3) << (int)widx.count << ", "
           << "widx: "  << std::setw(3) << (int)widx.widx
           << ")";
        return os;
    }
    #endif
};


//******************************************************************************
//
// Count Sliding Window
//
//******************************************************************************
template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int STEP
>
struct CountSliding
{
    static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);

    using counter_t = unsigned long;
    using count_t = uint_for<SIZE>;
    using widx_t = window_index_t<SIZE, STEP>;

    using state_t = CountBucket_t<window_functor_t, SIZE>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = CountWindowWrapper_t<tuple_t, SIZE>;
    using wrapper_result_t = CountWindowResult_t<result_t, SIZE>;

    counter_t counter;
    widx_t first_widx;

    REMOVE_INIT(bool valids[N]);
    REMOVE_INIT(state_t state[N]);

    CountSliding()
    : counter(0)
    , first_widx()
    {
        #pragma HLS ARRAY_PARTITION type=complete dim=1 variable=valids
        #pragma HLS ARRAY_PARTITION type=complete dim=1 variable=state

        COUNT_SLIDING_INIT:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            state[i] = state_t(i);
        }
    }

    void operator()(const wrapper_t & in, wrapper_result_t & out, bool & valid)
    {
        const tuple_t tuple = in.unwrap();
        const bool flush = in.is_flush();

        const wid_t last_wid = DIV_FLOOR(counter, STEP);

        CountSliding_UPDATE:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            if (state[i].wid <= last_wid) {
                valids[i] = state[i].update(tuple, flush, N);
            }
        }

        const auto widx = first_widx.get();
        const state_t & s = state[widx];

        valid = valids[widx];
        out = wrapper_result_t(0, s.state, s.wid, s.count);

        if (counter >= counter_t(SIZE - 1)) {
            first_widx.increment();
        }

        counter++;
    }
};

//******************************************************************************
//
// Keyed Count Sliding Window
//
//******************************************************************************
template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int STEP,
    unsigned int MAX_KEY
>
struct KeyedCountSliding
{
    static constexpr unsigned int N = DIV_CEIL(SIZE, STEP);
    
    using counter_t = unsigned long;
    using widx_t = window_index_t<SIZE, STEP>;

    using state_t = CountBucket_t<window_functor_t, SIZE>;
    using tuple_t = typename state_t::tuple_t;
    using result_t = typename state_t::result_t;

    using wrapper_t = CountWindowWrapper_t<tuple_t, SIZE>;
    using wrapper_result_t = CountWindowResult_t<result_t, SIZE>;

    key_t key;
    counter_t count;
    widx_t left_widx;

    REMOVE_INIT(bool valids[N]);
    REMOVE_INIT(state_t state[N]);
    
    REMOVE_INIT(bool initialized[MAX_KEY]);
    REMOVE_INIT(counter_t counts[MAX_KEY]);
    REMOVE_INIT(widx_t left_widxs[MAX_KEY]);
    
    REMOVE_INIT(state_t states[N][MAX_KEY]);

    KeyedCountSliding()
    : key(0)
    , count(0)
    , left_widx()
    {   
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=valids
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=state

        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=initialized
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=counts
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=left_widxs

        #pragma HLS BIND_STORAGE type=ram_s2p impl=bram variable=states
        #pragma HLS ARRAY_PARTITION dim=1 type=complete variable=states

        initialized[0] = true;
        KeyedCountSliding_INITIALIZED_INIT:
        for (unsigned int i = 1; i < MAX_KEY; ++i) {
            #pragma HLS UNROLL
            
            initialized[i] = false;
        }

        KeyedCountSliding_STATES_INIT:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            state[i] = state_t(i);
        }
    }

    void operator()(const wrapper_t & in, wrapper_result_t & out, bool & valid)
    {
        #pragma HLS DEPENDENCE dependent=false direction=raw type=intra variable=states

        const key_t tuple_key = in.get_key();
        const tuple_t tuple = in.unwrap();
        const bool flush = in.is_flush();

        if (key != tuple_key) {
            initialized[key] = true;
            counts[key]      = count;
            left_widxs[key]  = left_widx;

            KeyedCountSliding_STATES_STORE:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                states[i][key] = state[i];
            }

            const bool is_initiazlied = initialized[tuple_key];
            count     = is_initiazlied ? counts[tuple_key]     : counter_t(0);
            left_widx = is_initiazlied ? left_widxs[tuple_key] : widx_t();

            KeyedCountSliding_STATES_LOAD:
            for (unsigned int i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                
                state[i] = is_initiazlied ? states[i][tuple_key] : state_t(i);
            }
        }
        key = tuple_key;
        
        const wid_t last_wid = DIV_FLOOR(count, STEP);

        KeyedCountSliding_UPDATE:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL

            state_t & s = state[i];
            if (s.get_wid() <= last_wid) {
                valids[i] = s.update(tuple, flush, N);
            }
        }

        const auto widx = left_widx.get();
        const state_t & s = state[widx];

        valid = valids[widx];
        out = wrapper_result_t(key, s.state, s.wid, s.count);

        if (count >= counter_t(SIZE - 1)) {
            left_widx.increment();
        }

        count++;
    }
};

} // namespace Window
} // namespace fx

#endif // __COUNT_WINDOW_HPP__
