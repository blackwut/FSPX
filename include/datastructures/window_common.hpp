#ifndef __WINDOW_COMMON_HPP__
#define __WINDOW_COMMON_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"

#if !defined(__SYNTHESIS__)
#include <ostream>
#endif


namespace fx {

template <typename OP>
struct count_result_t
{
    using WIN_T  = unsigned int;
    using OUT_T  = typename OP::OUT_T;

    WIN_T wid;
    OUT_T value;

    count_result_t(
        const WIN_T wid,
        const OUT_T value
    )
    : wid(wid)
    , value(value)
    {}

    count_result_t()
    : count_result_t(WIN_T(-1), OP::lower(OP::identity()))
    {}

    count_result_t(const count_result_t & other)
    : count_result_t(other.wid, other.value)
    {}

    count_result_t & operator=(const count_result_t & other)
    {
    #pragma HLS INLINE
        wid = other.wid;
        value = other.value;
        return *this;
    }

    bool operator==(const count_result_t & other) const
    {
    #pragma HLS INLINE
        return wid == other.wid && value == other.value;
    }

    bool operator!=(const count_result_t & other) const
    {
    #pragma HLS INLINE
        return !(*this == other);
    }

    bool is_valid() const
    {
    #pragma HLS INLINE
        return wid != WIN_T(-1);
    }

    void reset()
    {
    #pragma HLS INLINE
        wid = WIN_T(-1);
        value = OP::lower(OP::identity());
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const count_result_t & result)
    {
        os << "(wid: "    << std::setw(3) << (int)result.wid
           << ", value: " << std::setw(3) << result.value << ")";

        return os;
    }
    #endif
};


template <typename OP, unsigned int SIZE>
struct count_state_t
{
    using WIN_T   = unsigned int;
    using IN_T    = typename OP::IN_T;
    using AGG_T   = typename OP::AGG_T;
    using COUNT_T = unsigned int;

    WIN_T wid;
    AGG_T value;
    COUNT_T count;

    count_state_t(
        const WIN_T wid,
        const AGG_T value,
        const COUNT_T count
    )
    : wid(wid)
    , value(value)
    , count(count)
    {}

    count_state_t()
    : count_state_t(WIN_T(-1), OP::identity(), COUNT_T(0))
    {}

    count_state_t(const count_state_t & other)
    : count_state_t(other.wid, other.value, other.count)
    {}

    count_state_t & operator=(const count_state_t & other)
    {
    #pragma HLS INLINE
        wid = other.wid;
        value = other.value;
        count = other.count;
        return *this;
    }

    bool operator==(const count_state_t & other) const
    {
    #pragma HLS INLINE
        return wid == other.wid && value == other.value && count == other.count;
    }

    bool operator!=(const count_state_t & other) const
    {
    #pragma HLS INLINE
        return !(*this == other);
    }

    bool is_empty() const
    {
    #pragma HLS INLINE
        return count == COUNT_T(0);
    }

    bool is_closing() const
    {
    #pragma HLS INLINE
        return count == (SIZE - 1);
    }

    bool is_valid() const
    {
    #pragma HLS INLINE
        return wid != WIN_T(-1);
    }

    void increment_count()
    {
    #pragma HLS INLINE
        count = is_closing() ? COUNT_T(0) : count + COUNT_T(1);
    }

    void update(const AGG_T _value)
    {
    #pragma HLS INLINE

        const AGG_T _agg = is_empty() ? OP::identity() : value;
        value = OP::combine(_agg, _value);
        increment_count();
    }

    void update(const IN_T _value)
    {
    #pragma HLS INLINE
        update(OP::lift(_value));
    }

    count_result_t<OP> to_result() const
    {
    #pragma HLS INLINE
        return count_result_t<OP>(wid, OP::lower(value));
    }

    void reset()
    {
    #pragma HLS INLINE
        wid = WIN_T(-1);
        value = OP::identity();
        count = COUNT_T(0);
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const count_state_t & state)
    {
        os << "(wid: "    << std::setw(3) << (int)state.wid
           << ", value: " << std::setw(3) << state.value
           << ", count: " << std::setw(3) << state.count << ")";
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
// *****************************************************************************


template <typename OP>
struct time_result_t
{
    using WIN_T  = unsigned int;
    using OUT_T  = typename OP::OUT_T;
    using TIME_T = unsigned int;

    WIN_T wid;
    OUT_T value;
    TIME_T timestamp;

    time_result_t(
        const WIN_T wid,
        const OUT_T value,
        const TIME_T timestamp
    )
    : wid(wid)
    , value(value)
    , timestamp(timestamp)
    {}

    time_result_t()
    : time_result_t(WIN_T(-1), OP::lower(OP::identity()), TIME_T(-1))
    {}

    time_result_t(const time_result_t & other)
    : time_result_t(other.wid, other.value, other.timestamp)
    {}

    time_result_t & operator=(const time_result_t & other)
    {
    #pragma HLS INLINE
        wid = other.wid;
        value = other.value;
        timestamp = other.timestamp;
        return *this;
    }

    bool is_valid() const
    {
    #pragma HLS INLINE
        return wid != WIN_T(-1);
    }

    void reset()
    {
    #pragma HLS INLINE
        wid = WIN_T(-1);
        value = OP::lower(OP::identity());
        timestamp = TIME_T(-1);
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const time_result_t & result)
    {
        os << "(wid: "        << std::setw(3) << (int)result.wid
           << ", value: "     << std::setw(3) << result.value
           << ", timestamp: " << std::setw(3) << (int)result.timestamp << ")";
        return os;
    }
    #endif
};


template <typename OP, typename KEY_T>
struct keyed_time_result_t
{
    using WIN_T  = unsigned int;
    using OUT_T  = typename OP::OUT_T;
    using TIME_T = unsigned int;
    using SEQ_T  = ap_uint<64>;

    WIN_T wid;
    KEY_T key;
    OUT_T value;
    TIME_T timestamp;
    TIME_T sequence;

    keyed_time_result_t(
        const WIN_T wid,
        const KEY_T key,
        const OUT_T value,
        const TIME_T timestamp,
        const TIME_T sequence
    )
    : wid(wid)
    , key(key)
    , value(value)
    , timestamp(timestamp)
    , sequence(sequence)
    {}

    keyed_time_result_t()
    : keyed_time_result_t(WIN_T(-1), KEY_T(-1), OP::lower(OP::identity()), TIME_T(-1), TIME_T(-1))
    {}

    keyed_time_result_t(const keyed_time_result_t & other)
    : keyed_time_result_t(other.wid, other.key, other.value, other.timestamp, other.sequence)
    {}

    keyed_time_result_t & operator=(const keyed_time_result_t & other)
    {
    #pragma HLS INLINE
        wid = other.wid;
        key = other.key;
        value = other.value;
        timestamp = other.timestamp;
        sequence = other.sequence;
        return *this;
    }

    bool is_valid() const
    {
    #pragma HLS INLINE
        return wid != WIN_T(-1);
    }

    void reset()
    {
    #pragma HLS INLINE
        wid = WIN_T(-1);
        key = KEY_T(-1);
        value = OP::lower(OP::identity());
        timestamp = TIME_T(-1);
        sequence = TIME_T(-1);
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const keyed_time_result_t & result)
    {
        os << "(wid: "        << std::setw(3) << (int)result.wid
           << ", key: "       << std::setw(3) << result.key
           << ", value: "     << std::setw(3) << result.value
           << ", timestamp: " << std::setw(3) << (int)result.timestamp
           << ", sequence: "  << std::setw(3) << (int)result.sequence << ")";

        return os;
    }
    #endif
};


template <typename OP>
struct time_state_t
{
    using WIN_T  = unsigned int;
    using AGG_T  = typename OP::AGG_T;
    using TIME_T = unsigned int;

    WIN_T wid;
    AGG_T value;
    TIME_T timestamp;

    // time_state_t(const WIN_T wid, const AGG_T value, const TIME_T timestamp)
    // : wid(wid)
    // , value(value)
    // , timestamp(timestamp)
    // {}

    // time_state_t()
    // : time_state_t(WIN_T(-1), OP::identity(), TIME_T(-1))
    // {}

    // time_state_t(const time_state_t & other)
    // : time_state_t(other.wid, other.value, other.timestamp)
    // {}

    time_state_t & operator=(const time_state_t & other)
    {
    #pragma HLS INLINE
        wid = other.wid;
        value = other.value;
        timestamp = other.timestamp;
        return *this;
    }

    bool is_valid() const
    {
    #pragma HLS INLINE
        return wid != WIN_T(-1);
    }

    void reset()
    {
    #pragma HLS INLINE
        wid = WIN_T(-1);
        value = OP::identity();
        timestamp = TIME_T(-1);
    }

    time_result_t<OP> to_result() const
    {
    #pragma HLS INLINE
        return time_result_t<OP>(wid, OP::lower(value), timestamp);
    }

    template <typename KEY_T>
    keyed_time_result_t<OP, KEY_T> to_result_key(const KEY_T key, const TIME_T sequence) const
    {
    #pragma HLS INLINE
        return keyed_time_result_t<OP, KEY_T>(wid, key, OP::lower(value), timestamp, sequence);
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const time_state_t & state)
    {
        os << "(wid: "        << std::setw(3) << (int)state.wid
           << ", value: "     << std::setw(3) << state.value
           << ", timestamp: " << std::setw(3) << (int)state.timestamp << ")";

        return os;
    }
    #endif
};


} // namespace fx

#endif // __WINDOW_COMMON_HPP__
