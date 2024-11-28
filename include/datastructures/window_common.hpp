#ifndef __WINDOW_COMMON_HPP__
#define __WINDOW_COMMON_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"

#if !defined(__SYNTHESIS__)
#include <ostream>
#endif


namespace fx {

//*****************************************************************************
//
//
// COUNT STRUCTURES
//
//
//*****************************************************************************

template <
    typename window_functor_t,
    unsigned int SIZE
>
struct CountBucket_t
{
    using count_t = uint_for<SIZE + 1>;
    using tuple_t = arg_t(0, window_functor_t::operator());
    using result_t = arg_t(1, window_functor_t::operator());

    wid_t wid;
    result_t state;
    count_t count;
    
    CountBucket_t()
    : wid(wid_t(-1))
    , state()
    , count(count_t(SIZE))
    {}

    CountBucket_t(const wid_t wid)
    : wid(wid)
    , state()
    , count(count_t(SIZE))
    {}

    CountBucket_t (
        const wid_t wid,
        const count_t count
    )
    : wid(wid)
    , state()
    , count(count)
    {}

    CountBucket_t (
        const wid_t wid,
        const result_t state,
        const count_t count
    )
    : wid(wid)
    , state(state)
    , count(count)
    {}

    CountBucket_t operator=(const CountBucket_t & other)
    {
        #pragma HLS INLINE
        
        wid = other.wid;
        count = other.count;
        state = other.state;
        return *this;
    }

    bool is_empty() const
    {
        #pragma HLS INLINE
        return count == count_t(SIZE);
    }

    bool update(
        const tuple_t & tuple,
        const bool flush,
        const wid_t multiplier = 1
    )
    {
        #pragma HLS INLINE
        
        static window_functor_t window_functor;
        const bool empty = is_empty();
        const bool valid = (SIZE == 1 || count == count_t(SIZE - 1));

        result_t tmp = empty ? result_t() : state;
        window_functor(tuple, tmp);

        if (!flush) {
            if (count == count_t(SIZE)) {
                count = count_t(1);
                wid += multiplier;
            } else {
                count++;
            }
            state = tmp;
        }

        return valid || (flush && !empty);
    }

    // bool is_closing() const
    // {
    //     #pragma HLS INLINE
    //     return count == count_t(SIZE - 1);
    // }

    // void increment_count (
    //     const bool valid,
    //     const unsigned int multiplier = 1
    // )
    // {
    //     #pragma HLS INLINE
        
    //     if (valid) {
    //         wid += multiplier;
    //         count = count_t(0);
    //     } else {
    //         count++;
    //     }
    // }

    // bool update (
    //     const tuple_t & tuple,
    //     const bool flush,
    //     const unsigned int multiplier = 1
    // )
    // {
    //     #pragma HLS INLINE
        
    //     static window_functor_t window_functor;

    //     result_t tmp;
    //     const bool empty = is_empty();

    //     if (!empty) {
    //         tmp = state;
    //     } 
    //     window_functor(tuple, tmp);

    //     if (!flush) {
    //         state = tmp;
    //     }

    //     const bool valid = is_closing() || (flush && !empty);
    //     increment_count(valid, multiplier);

    //     std::cout << count << std::endl;

    //     return valid;
    // }

    // wid_t get_wid() const
    // {
    //     #pragma HLS INLINE
    //     return wid;
    // }

    // result_t get_result() const
    // {
    //     #pragma HLS INLINE
    //     return state;
    // }

    // count_t get_count() const
    // {
    //     #pragma HLS INLINE
    //     return count;
    // }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const CountBucket_t & state)
    {
        os << "("
           << "wid: "   << std::setw(3) << (int)state.wid   << ", "
           << "state: " << state.state                      << ", "
           << "count: " << std::setw(3) << (int)state.count
           << ")";
        return os;
    }
    #endif
};

template <
    typename T,
    unsigned int SIZE
>
struct CountWindowWrapper_t
{
    using tuple_t = T;

    key_t key;
    tuple_t tuple;
    bool flush;

    CountWindowWrapper_t()
    : key(-1) // TODO: verify that -1 is ok or put back 0
    , tuple()
    , flush(true)
    {}

    CountWindowWrapper_t(const key_t key)
    : key(key)
    , tuple()
    , flush(true)
    {}

    CountWindowWrapper_t(const tuple_t & tuple)
    : key(-1)
    , tuple(tuple)
    , flush(false)
    {}

    CountWindowWrapper_t (
        const key_t key,
        const tuple_t tuple
    )
    : key(key)
    , tuple(tuple)
    , flush(false)
    {}

    CountWindowWrapper_t (
        const key_t key,
        const tuple_t & tuple,
        const bool flush
    )
    : key(key)
    , tuple(tuple)
    , flush(flush)
    {}

    CountWindowWrapper_t & operator=(const CountWindowWrapper_t & other)
    {
        #pragma HLS INLINE
        
        key = other.key;
        tuple = other.tuple;
        flush = other.flush;
        return *this;
    }

    key_t get_key() const
    {
        #pragma HLS INLINE
        return key;
    }

    tuple_t unwrap() const
    {
        #pragma HLS INLINE
        return tuple;
    }

    const tuple_t & unwrap()
    {
        #pragma HLS INLINE
        return tuple;
    }

    bool is_flush() const
    {
        #pragma HLS INLINE
        return flush;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const CountWindowWrapper_t & wrapper)
    {
        os << "("
           << "key: "   << std::setw(3) << (int)wrapper.key     << ", "
           << "tuple: " << wrapper.tuple                        << ", "
           << "flush: " << (wrapper.flush ? "true" : "false")
           << ")";
        return os;
    }
    #endif
};

template <
    typename T,
    unsigned int SIZE
>
struct CountWindowResult_t
{
    using tuple_t = T;
    using count_t = uint_for<SIZE + 1>;

    key_t key;
    tuple_t tuple;
    wid_t wid;
    count_t count;

    CountWindowResult_t()
    : key(-1)
    , tuple()
    , wid(-1)
    , count(0)
    {}

    CountWindowResult_t (
        const key_t key,
        const tuple_t & tuple,
        const wid_t wid,
        const count_t count
    )
    : key(key)
    , tuple(tuple)
    , wid(wid)
    , count(count)
    {}

    CountWindowResult_t & operator=(const CountWindowResult_t & other)
    {
        #pragma HLS INLINE
        
        key = other.key;
        tuple = other.tuple;
        wid = other.wid;
        count = other.count;
        return *this;
    }

    key_t get_key() const
    {
        #pragma HLS INLINE
        return key;
    }

    tuple_t unwrap() const
    {
        #pragma HLS INLINE
        return tuple;
    }

    wid_t get_wid() const
    {
        #pragma HLS INLINE
        return wid;
    }

    count_t get_count() const
    {
        #pragma HLS INLINE
        return count;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const CountWindowResult_t & result)
    {
        os << "("
           << "key: "   << std::setw(3) << (int)result.key   << ", "
           << "tuple: " << result.tuple                      << ", "
           << "wid: "   << std::setw(3) << (int)result.wid   << ", "
           << "count: " << std::setw(3) << (int)result.count
           << ")";
        return os;
    }
    #endif
};


//*****************************************************************************
//
//
// TIME STRUCTURES
//
//
//*****************************************************************************

template <typename window_functor_t>
struct TimeBucket_t
{
    using tuple_t  = arg_t(0, window_functor_t::operator());
    using result_t = arg_t(1, window_functor_t::operator());

    wid_t wid;
    result_t state;

    TimeBucket_t()
    : wid(wid_t(-1))
    , state()
    {}

    TimeBucket_t(const wid_t wid)
    : wid(wid)
    , state()
    {}

    TimeBucket_t (
        const wid_t wid,
        const result_t state
    )
    : wid(wid)
    , state(state)
    {}

    TimeBucket_t operator=(const TimeBucket_t & other)
    {
        #pragma HLS INLINE

        wid = other.wid;
        state = other.state;
        return *this;
    }

    void update(
        const tuple_t & tuple,
        const wid_t new_wid
    )
    {
        #pragma HLS INLINE

        static window_functor_t window_functor;

        result_t tmp = wid != new_wid ? result_t() : state;
        window_functor(tuple, tmp);

        wid = new_wid;
        state = tmp;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const TimeBucket_t & state)
    {
        os << "("
           << "wid: "   << std::setw(3) << (int)state.wid << ", "
           << "state: " << state.state
           << ")";
        return os;
    }
    #endif
};

template <
    typename T,
    unsigned int SIZE,
    unsigned int STEP
>
struct TimeWindowWrapper_t
{
    using tuple_t = T;

    key_t key;
    tuple_t tuple;
    bool flush;
    wid_t first;
    wid_t last;

    wid_t calculate_first(const timestamp_t timestamp) const
    {
        #pragma HLS INLINE
        return wid_t(timestamp < SIZE ? 0 : DIV_CEIL(timestamp - SIZE + 1, STEP));
    }

    wid_t calculate_last(const timestamp_t timestamp) const
    {
        #pragma HLS INLINE
        return wid_t(DIV_FLOOR(timestamp, STEP));
    }

    TimeWindowWrapper_t()
    : key(0) // TODO: check if 0 is ok or put back -1
    , tuple()
    , flush(true)
    , first(0)
    , last(-1)
    {}

    TimeWindowWrapper_t(const key_t key)
    : key(key)
    , tuple()
    , flush(true)
    , first(0)
    , last(-1)
    {}

    TimeWindowWrapper_t(const tuple_t tuple)
    : key(0)
    , tuple(tuple)
    , flush(false)
    , first(calculate_first(tuple.timestamp))
    , last(calculate_last(tuple.timestamp))
    {}

    TimeWindowWrapper_t (
        const key_t key,
        const tuple_t & tuple
    )
    : key(key)
    , tuple(tuple)
    , flush(false)
    , first(calculate_first(tuple.timestamp))
    , last(calculate_last(tuple.timestamp))
    {}

    TimeWindowWrapper_t & operator=(const TimeWindowWrapper_t & other)
    {
        #pragma HLS INLINE

        key = other.key;
        tuple = other.tuple;
        flush = other.flush;
        first = other.first;
        last = other.last;
        return *this;
    }

    key_t get_key() const
    {
        #pragma HLS INLINE
        return key;
    }

    tuple_t unwrap() const
    {
        #pragma HLS INLINE
        return tuple;
    }

    timestamp_t get_timestamp() const
    {
        #pragma HLS INLINE
        return tuple.timestamp;
    }

    bool is_flush() const
    {
        #pragma HLS INLINE
        return flush;
    }

    wid_t get_first() const
    {
        #pragma HLS INLINE
        return first;
    }

    wid_t get_last() const
    {
        #pragma HLS INLINE
        return last;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const TimeWindowWrapper_t & wrapper)
    {
        os << "("
           << "key: "   << std::setw(3) << (int)wrapper.key   << ", "
           << "tuple: " << wrapper.tuple                      << ", "
           << "flush: " << (wrapper.flush ? "true" : "false") << ", "
           << "first: " << std::setw(3) << (int)wrapper.first << ", "
           << "last: "  << std::setw(3) << (int)wrapper.last
           << ")";
        return os;
    }
    #endif
};

template <
    typename T,
    unsigned int SIZE,
    unsigned int STEP
>
struct TimeWindowResult_t
{
    using tuple_t = T;
    
    key_t key;
    tuple_t tuple;
    seq_t sequence;
    wid_t wid;
    timestamp_t timestamp;

    timestamp_t calculate_timestamp(const wid_t wid) const
    {
        #pragma HLS INLINE
        return wid * STEP + SIZE - 1;
    }

    TimeWindowResult_t()
    : key(-1)
    , tuple()
    , sequence(0)
    , wid(-1)
    , timestamp(calculate_timestamp(-1))
    {}

    TimeWindowResult_t(const tuple_t tuple)
    : key(0)
    , tuple(tuple)
    , sequence(0)
    , wid(0)
    , timestamp(calculate_timestamp(0))
    {}

    TimeWindowResult_t (
        const key_t key,
        const tuple_t tuple,
        const seq_t sequence = 0,
        const wid_t wid = 0
    )
    : key(key)
    , tuple(tuple)
    , sequence(sequence)
    , wid(wid)
    , timestamp(calculate_timestamp(wid))
    {}

    TimeWindowResult_t & operator=(const TimeWindowResult_t & other)
    {
        #pragma HLS INLINE

        key = other.key;
        tuple = other.tuple;
        sequence = other.sequence;
        wid = other.wid;
        timestamp = other.timestamp;
        return *this;
    }

    key_t get_key() const
    {
        #pragma HLS INLINE
        return key;
    }

    tuple_t unwrap() const
    {
        #pragma HLS INLINE
        return tuple;
    }

    seq_t get_sequence() const
    {
        #pragma HLS INLINE
        return sequence;
    }

    wid_t get_wid() const
    {
        #pragma HLS INLINE
        return wid;
    }

    timestamp_t get_timestamp() const
    {
        #pragma HLS INLINE
        return timestamp;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const TimeWindowResult_t & result)
    {
        os << "("
           << "key: "       << std::setw(3) << (int)result.key      << ", "
           << "tuple: "     << result.tuple                         << ", "
           << "sequence: "  << std::setw(3) << (int)result.sequence << ", "
           << "wid: "       << std::setw(3) << (int)result.wid      << ", "
           << "timestamp: " << std::setw(3) << (int)result.timestamp
           << ")";
        return os;
    }
    #endif
};


//*****************************************************************************
//
//
// WINDOW FORWARD AND FLUSH
//
//
//*****************************************************************************

template <
    typename wrapper_t,
    typename stream_in_t,
    typename stream_out_t
>
void WindowForwardFlush (
    stream_in_t & stream_in,
    stream_out_t & steram_out
)
{
    using tuple_t = typename stream_in_t::data_t;

    bool last = stream_in.read_eos();

    WindowForward:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        const tuple_t in = stream_in.read();
        last = stream_in.read_eos();

        const wrapper_t out(in);
        steram_out.write(out);
    }

    const wrapper_t out;
    steram_out.write(out);
    steram_out.write_eos();
}

template <
    typename wrapper_t,
    unsigned int MAX_KEY,
    typename key_extractor_t,
    typename stream_in_t,
    typename stream_out_t
>
void KeyedWindowForwardFlush (
    stream_in_t & stream_in,
    stream_out_t & steram_out,
    key_extractor_t && key_extractor
)
{
    using tuple_t = typename stream_in_t::data_t;

    bool last = stream_in.read_eos();

    KeyedWindowForward:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        const tuple_t in = stream_in.read();
        last = stream_in.read_eos();

        const key_t key = key_extractor(in);
        const wrapper_t out(in, key);
        steram_out.write(out);
    }

    KeyedWindowFlush:
    for (unsigned int i = 0; i < MAX_KEY; ++i) {
    #pragma HLS PIPELINE II = 1
        const key_t key = i;
        const wrapper_t out(key);
        steram_out.write(out);
    }
    steram_out.write_eos();
}

} // namespace fx

#endif // __WINDOW_COMMON_HPP__
