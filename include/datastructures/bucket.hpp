#ifndef __BUCKET_HPP__
#define __BUCKET_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"
#include "window_common.hpp"


namespace fx {

template <typename OP, unsigned int SIZE>
struct count_bucket_t
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using WIN_T   = unsigned int;
    using COUNT_T = unsigned int;

    WIN_T wid;
    count_state_t<OP, SIZE> state;

    count_bucket_t()
    : wid(0)
    , state()
    {}

    void reset(const WIN_T w, const count_state_t<OP, SIZE> s)
    {
        wid = w;
        state = s;
    }

    void insert(const IN_T in, count_result_t<OP> & result, bool & valid)
    {
        // const bool _first = (state.count == 0);
        // const bool _valid = (state.count == (SIZE - 1));

        // // ++state.count % SIZE
        // state.count = _valid ? 0 : COUNT_T(state.count + 1);

        // // update the aggregate
        // const AGG_T _lifted = OP::lift(in);
        // const AGG_T _agg = _first ? OP::identity() : state.agg;
        // state.agg = OP::combine(_agg, _lifted);

        // // output the result
        // result.value = OP::lower(state.agg);
        // result.valid = _valid;

        valid = state.is_closing();
        state.update(in);
        result = state.get_result();
    }
};

#if 0
// bucket time per stream di tuple ordinate in base al timestamp
template <typename OP, unsigned int SIZE>
struct time_bucket_t
{
    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    struct count_state_t {
        WIN_T wid;
        TIME_T ts;
        AGG_T agg;

        state_t(const WIN_T wid = 0, const TIME_T ts = 0, const AGG_T agg = OP::identity())
        : wid(wid)
        , ts(ts)
        , agg(agg)
        {}
    };

    struct result_t {
        OUT_T value;
        TIME_T timestamp;
        bool valid;
    };

    TIME_T offset;
    state_t state;

    time_bucket_t(const TIME_T offset = 0)
    : offset(offset)
    , state()
    {}

    state_t get_state() const
    {
        return state;
    }

    void set_state(const state_t new_state)
    {
        state = new_state;
    }

    void insert(const IN_T in, const TIME_T timestamp, result_t & result)
    {
        // assert(timestamp >= offset)
        const WIN_T _wid = (timestamp - offset) / SIZE;
        const bool _valid = _wid > state.wid;

        const AGG_T last_agg = state.agg;

        // update the aggregate
        const AGG_T _lifted = OP::lift(in);
        const AGG_T _agg = _valid ? OP::identity() : last_agg;
        state.agg = OP::combine(_agg, _lifted);

        if (_valid) {
            result.value = OP::lower(last_agg);
            result.timestamp = state.ts;
            result.valid = true;

            state.wid = (timestamp - offset) / SIZE;
            state.ts = timestamp;
        } else {
            result.value = OP::lower(OP::identity());
            result.timestamp = 0;
            result.valid = false;
        }
    }
};
#endif

template <typename OP, unsigned int SIZE, unsigned int LATENESS>
struct late_bucket_t
{
    static constexpr unsigned int N = (1 + (LATENESS + SIZE - 1) / SIZE);

    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    WIN_T left_idx;
    TIME_T max_timestamp;
    WIN_T max_wid;

    time_state_t<OP> states[N];

    time_state_t<OP> compact_data[N][N];

    WIN_T tail;
    time_state_t<OP> shift_reg[N];

    late_bucket_t()
    : left_idx(0)
    , max_timestamp(LATENESS)
    , max_wid(N - 1)
    , tail(0)
    {
        #pragma HLS ARRAY_PARTITION variable = states complete
        #pragma HLS ARRAY_PARTITION variable = compact_data complete
        #pragma HLS ARRAY_PARTITION variable = shift_reg complete

        for (WIN_T i = 0; i < N; ++i) {
            states[i].reset();
            shift_reg[i].reset();
        }

        for (WIN_T i = 0; i < N; ++i) {
            for (WIN_T j = 0; j < N; ++j) {
                compact_data[i][j].reset();
            }
        }
    }

    void copy_states_to_compact_data(const WIN_T old_left_idx, const WIN_T len)
    {
        PROCESS_COPY:
        for (WIN_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            if (i < len) {
                if (old_left_idx + i < N) {
                    compact_data[0][i] = states[old_left_idx + i];
                } else {
                    compact_data[0][i] = states[old_left_idx + i - N];
                }
            } else {
                compact_data[0][i] = time_state_t<OP>();
            }
        }
    }

    void compact(const time_state_t<OP> in[N], const WIN_T left_wid, time_state_t<OP> out[N])
    {
        ap_uint<N> move = 0;
        COMPACT_MOVE:
        for (WIN_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            move[i] = (in[i].wid < left_wid);
        }

        COMPACT_WRITE:
        for (WIN_T i = 0; i < (N - 1); ++i) {
        #pragma HLS UNROLL

            // WIN_T p = 1 << i;
            // WIN_T m = move.range(i, 0);
            // std::cout << i << ") " << p << " vs " << m << std::endl;

            if (move.range(i, 0) == 0) {
                out[i] = in[i];
            } else {
                out[i] = in[i + 1];
            }
        }

        if (move.range(N - 1, 0) == 0) {
            out[N - 1] = in[N - 1];
        } else {
            out[N - 1] = time_state_t<OP>();
        }
    }

    void compact_states(const WIN_T left_wid)
    {
        GENERATE_COMPACT:
        for (WIN_T i = 0; i < (N - 1); ++i) {
        #pragma HLS UNROLL
            compact(compact_data[i], left_wid, compact_data[i + 1]);
        }
    }

    template <typename STREAM_OUT>
    void ship_result(const time_state_t<OP> in[N], const WIN_T len, STREAM_OUT & ostrm)
    {
        const WIN_T old_tail = tail;
        const WIN_T tmp_last = tail + len;
        tail = (tmp_last > 0) ? tmp_last - 1 : 0;

        const time_state_t<OP> state = shift_reg[0];
        if (state.is_valid()) {
            time_result_t<OP> result(state.wid, OP::lower(state.value), state.timestamp);
            ostrm.write(result);
        }

        SHIFT_SHIPPER:
        for (WIN_T i = 0; i < (N - 1); ++i) {
        #pragma HLS UNROLL
            if (i < old_tail) {
                shift_reg[i] = shift_reg[i + 1];
            } else if (i - old_tail < len) {
                shift_reg[i] = in[i - old_tail];
            } else {
                shift_reg[i] = time_state_t<OP>();
            }
        }
        
        if (len < N) {
            shift_reg[N - 1] = time_state_t<OP>();
        } else {
            shift_reg[N - 1] = in[N - old_tail - 1];
        }
    }

    void update_state(const IN_T in, const TIME_T timestamp, const WIN_T wid, const WIN_T wid_idx)
    {
        const bool first_insert = (states[wid_idx].wid != wid);

        const AGG_T last_agg = states[wid_idx].value;
        const AGG_T _lifted  = OP::lift(in);
        const AGG_T _agg     = first_insert ? OP::identity() : last_agg;

        states[wid_idx].wid       = wid;
        states[wid_idx].value     = OP::combine(_agg, _lifted);
        states[wid_idx].timestamp = first_insert ? timestamp : states[wid_idx].timestamp;
    }

    template <typename STREAM_OUT>
    void process(const IN_T in, const TIME_T timestamp, const bool valid, STREAM_OUT & ostrm)
    {
        const WIN_T _wid = timestamp / SIZE;
        const WIN_T _wid_idx = _wid % N;
        const bool _drop = !valid || (timestamp < max_timestamp - LATENESS);

        const WIN_T _left_wid = max_wid - N + 1;
        const WIN_T _left_idx = left_idx;

        WIN_T _len = 0;
        bool _fire = false;
        if (_wid > max_wid) {
            _fire = valid;
            
            _len = _wid - max_wid;
            if (_len > N) {
                _len = N;
            }
            
            max_wid = _wid;

            if (_wid_idx == N - 1) {
                left_idx = 0;
            } else {
                left_idx = _wid_idx + 1;
            }
        }

        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;
        
        copy_states_to_compact_data(_left_idx, _len);
        if (!_drop) {
            update_state(in, timestamp, _wid, _wid_idx);
        }
        compact_states(_left_wid);

        const WIN_T _len_fire = (_fire ? _len : 0);
        ship_result(compact_data[N - 1], _len_fire, ostrm);
    }

    template <typename STREAM_OUT>
    void flush(STREAM_OUT & ostrm)
    {
        const WIN_T _left_wid = max_wid - N + 1;
        copy_states_to_compact_data(left_idx, N);
        compact_states(_left_wid);

        for (WIN_T i = 0; i <= N; ++i) {
            ship_result(compact_data[N - 1], (i == 0 ? N : 0), ostrm);
        }
    }
};

} // namespace fx

#endif // __BUCKET_HPP__
