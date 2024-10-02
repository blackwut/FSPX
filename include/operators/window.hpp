#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"
#include "../datastructures/window_common.hpp"
#include "../datastructures/bucket.hpp"


namespace fx {

template <typename OP, unsigned int KEYS, typename STREAM_IN, typename STREAM_OUT, typename STREAM_VALID>
void send_and_flush(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    STREAM_VALID & vstrm
)
{
    using T_IN = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();
    
    SEND:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        T_IN in;
        in.key = -1;
        in.timestamp = -1;
        bool valid = false;

        if (!istrm.empty() && !istrm.empty_eos()) {
            in = istrm.read();
            last = istrm.read_eos();
            valid = true;
        }

        ostrm.write(in);
        vstrm.write(valid);
    }

    FLUSH:
    for (unsigned int k = 0; k < KEYS; ++k) {
    #pragma HLS PIPELINE
        T_IN in;
        in.key = k;
        in.timestamp = -1;
        ostrm.write(in);
        vstrm.write(false);
    }

    ostrm.write_eos();
}


template <typename OP, unsigned int SIZE, unsigned int LATENESS>
struct _late_bucket_t
{
    static constexpr unsigned int N = (1 + (LATENESS + SIZE - 1) / SIZE);

    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;

    WIN_T left_wid;
    WIN_T right_wid;

    WIN_T left_idx;
    TIME_T max_timestamp;
    WIN_T max_wid;

    time_state_t<OP> states[N];


    _late_bucket_t()
    : left_wid(0)
    , right_wid(0)
    , left_idx(0)
    , max_timestamp(LATENESS)
    , max_wid(N - 1)
    {
        #pragma HLS array_partition variable = states complete
        // for (WIN_T i = 0; i < N; ++i) {
        //     states[i].reset();
        // }
    }

    template <typename STREAM_OUT>
    void send_results(const WIN_T _left_wid, const WIN_T _right_wid, STREAM_OUT ostrms[N])
    {
    #pragma HLS INLINE
        SEND_RESULTS:
        for (WIN_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            const time_state_t<OP> state = states[i];
            const WIN_T _wid = state.wid;
            if (_wid >= _left_wid && _wid < _right_wid) {
                ostrms[i].write(state.to_result());
            }
        }
    }

    void update_state(const IN_T in, const TIME_T timestamp, const WIN_T wid, const WIN_T wid_idx)
    {
    #pragma HLS INLINE

        const time_state_t<OP> state = states[wid_idx];

        const bool first_insert = (state.wid != wid);
        const AGG_T agg = first_insert ? OP::identity() : state.value;

        states[wid_idx] = time_state_t<OP>(
                            wid,
                            OP::combine(agg, OP::lift(in)),
                            first_insert ? timestamp : state.timestamp
        );
    }

    
    template <typename STREAM_OUT>
    void _process(const IN_T in, const TIME_T timestamp, const bool valid, STREAM_OUT ostrms[N])
    {
    #pragma HLS INLINE
        const WIN_T _wid = timestamp / SIZE;
        const WIN_T _wid_idx = _wid % N;
        const bool _drop = !valid || (timestamp < max_timestamp - LATENESS);

        const WIN_T _left_wid = left_wid;

        max_wid = (_wid > max_wid) ? _wid : max_wid;
        max_timestamp = (timestamp > max_timestamp) ? timestamp : max_timestamp;

        left_wid = max_wid - N + 1;

        send_results(_left_wid, left_wid, ostrms);
        if (!_drop) {
            update_state(in, timestamp, _wid, _wid_idx);
        }
    }

    template <typename STREAM_IN, typename STREAM_VALID, typename STREAM_OUT>
    void process(STREAM_IN & istrm, STREAM_VALID & vstrm, STREAM_OUT ostrms[N])
    {
        using T_IN  = typename STREAM_IN::data_t;

        bool last = istrm.read_eos();
        TIME_BUCKET_WHILE:
        while (!last) {
        #pragma HLS PIPELINE II = 1 // TODO: II = L
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

            T_IN in = istrm.read();
            bool valid = vstrm.read();

            last = istrm.read_eos();

            _process(in.value, in.timestamp, valid, ostrms);
        }

        TIME_BUCKET_EOS:
        for (WIN_T i = 0; i < N; ++i) {
            ostrms[i].write_eos();
        }
    }
};

// N = 16 ClockPeriod: 2.482 ns  ElapsedTime:   61.53 secondi  Memory: 1,208 GB  FMAX: 402.83 MHz
// N = 32 ClockPeriod: 2.482 ns  ElapsedTime:  123.06 secondi  Memory: 2,416 GB  FMAX: 402.83 MHz
// N = 64 ClockPeriod: 2.482 ns  ElapsedTime:  246.12 secondi  Memory: 4,832 GB  FMAX: 402.83 MHz
// N = 128 ClockPeriod: 2.482 ns  ElapsedTime:  492.24 secondi  Memory: 9,664 GB  FMAX: 402.83 MHz
// N = 256 ClockPeriod: 2.482 ns  ElapsedTime:  984.48 secondi  Memory: 19,328 GB  FMAX: 402.83 MHz
// N = 512 ClockPeriod: 2.482 ns  ElapsedTime: 1968.96 secondi  Memory: 38,656 GB  FMAX: 402.83 MHz
// N = 1024 ClockPeriod: 2.482 ns  ElapsedTime: 3937.92 secondi  Memory: 77,312 GB  FMAX: 402.83 MHz

template <typename OP, unsigned int KEYS, unsigned int SIZE, unsigned int LATENESS>
struct _keyed_late_bucket_t
{
    static constexpr unsigned int L = OP::LATENCY;
    static constexpr unsigned int N = (1 + (LATENESS + SIZE - 1) / SIZE);

    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T = unsigned int;
    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;
    using SEQ_T = ap_uint<64>;

    SEQ_T sequence;

    bool is_initalized[KEYS];
    WIN_T left_wid[KEYS];
    TIME_T max_timestamp[KEYS];
    WIN_T max_wid[KEYS];
    time_state_t<OP> states[N][KEYS];

    WIN_T curr_key;
    WIN_T curr_left_wid;
    TIME_T curr_max_timestamp;
    WIN_T curr_max_wid;
    time_state_t<OP> curr_states[N];


    _keyed_late_bucket_t()
    : sequence(0)
    , curr_key(-1)
    , curr_left_wid(0)
    , curr_max_timestamp(LATENESS)
    , curr_max_wid(N - 1)
    {
        #pragma HLS array_partition variable=is_initalized  type=complete
        #pragma HLS array_partition variable=left_wid       type=complete
        #pragma HLS array_partition variable=max_timestamp  type=complete
        #pragma HLS array_partition variable=max_wid        type=complete

        #pragma HLS bind_storage    variable=states         type=RAM_S2P  impl=BRAM
        #pragma HLS array_partition variable=states         type=complete dim=1

        #pragma HLS array_partition variable=curr_states    type=complete

        KEYED_LATE_BUCKET_INIT:
        for (KEY_T k = 0; k < KEYS; ++k) {
        #pragma HLS UNROLL
            is_initalized[k] = false;
        }
    }

    template <typename STREAM_OUT>
    void _process(const KEY_T key, const IN_T in, const TIME_T timestamp, const bool valid, STREAM_OUT ostrms[N])
    {
    #pragma HLS INLINE
    #pragma HLS dependence variable=states type=intra direction=RAW false

        const WIN_T _wid = timestamp / SIZE;
        const WIN_T _wid_idx = _wid % N; // TODO: controllare se e' piu' giusto (_wid - 1) % N

        if (curr_key != key) {

            if (curr_key != -1) {
                // store values for old key
                is_initalized[curr_key] = true;
                left_wid[curr_key]      = curr_left_wid;
                max_timestamp[curr_key] = curr_max_timestamp;
                max_wid[curr_key]       = curr_max_wid;

                PROCESS_STORE_STATES:
                for (WIN_T i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                    states[i][curr_key] = curr_states[i];
                }
            }

            curr_key = key;

            const bool _is_initialized = is_initalized[key];
            curr_left_wid = (_is_initialized) ? left_wid[key] : 0;
            curr_max_timestamp = (_is_initialized) ? max_timestamp[key] : LATENESS;
            curr_max_wid = (_is_initialized) ? max_wid[key] : N - 1;

            PROCESS_INIT_LOAD_STATES:
            for (WIN_T i = 0; i < N; ++i) {
            #pragma HLS UNROLL
                curr_states[i].wid       = (_is_initialized) ? states[i][key].wid       : WIN_T(-1);
                curr_states[i].value     = (_is_initialized) ? states[i][key].value     : OP::identity();
                curr_states[i].timestamp = (_is_initialized) ? states[i][key].timestamp : TIME_T(-1);
            }
        }

        const bool _drop = !valid || (timestamp < curr_max_timestamp - LATENESS);
        const WIN_T old_left_wid = curr_left_wid;

        curr_left_wid = (_wid > curr_max_wid) ? (_wid - N + 1) : (curr_max_wid - N + 1);
        curr_max_wid = (_wid > curr_max_wid) ? _wid : curr_max_wid;
        curr_max_timestamp = (timestamp > curr_max_timestamp) ? timestamp : curr_max_timestamp;

        SEND_RESULTS:
        for (WIN_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            const time_state_t<OP> state = curr_states[i];
            if (state.wid >= old_left_wid && state.wid < curr_left_wid) {
                ostrms[i].write(state.to_result_key(key, sequence));
            }
        }
        sequence++;

        if (!_drop) {
            UPDATE_STATE:
            for (WIN_T i = 0; i < N; ++i) {
            #pragma HLS UNROLL
                const time_state_t<OP> state = curr_states[i];
                const bool first_insert = (state.wid != _wid);
                const AGG_T agg = first_insert ? OP::identity() : state.value;

                if (i == _wid_idx) {
                    curr_states[i].wid = _wid;
                    curr_states[i].value = OP::combine(agg, OP::lift(in));
                    curr_states[i].timestamp = first_insert ? timestamp : state.timestamp;
                }
            }
        }
    }

    template <
        typename STREAM_IN,
        typename STREAM_VALID,
        typename STREAM_OUT,
        typename KEY_EXTRACTOR_T
    >
    void process(STREAM_IN & istrm, STREAM_VALID & vstrm, STREAM_OUT ostrms[N], KEY_EXTRACTOR_T && key_extractor)
    {
        using T_IN  = typename STREAM_IN::data_t;

        bool last = istrm.read_eos();
        TIME_BUCKET_WHILE:
        while (!last) {
        #pragma HLS PIPELINE II = L
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

            const T_IN in = istrm.read();
            auto key = key_extractor(in);
            bool valid = vstrm.read();

            last = istrm.read_eos();

            _process(key, in.value, in.timestamp, valid, ostrms);
        }

        TIME_BUCKET_EOS:
        for (WIN_T i = 0; i < N; ++i) {
            ostrms[i].write_eos();
        }
    }
};

template <typename OP, unsigned int KEYS, unsigned int SIZE, unsigned int STEP, unsigned int LATENESS>
struct _keyed_late_sliding_bucket_t
{
    static constexpr unsigned int L = OP::LATENCY;
    static constexpr unsigned int N = DIV_CEIL(SIZE + LATENESS, STEP);

    using IN_T  = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    using KEY_T = unsigned int;
    using TIME_T = unsigned int;
    using WIN_T  = unsigned int;
    using SEQ_T = ap_uint<64>;

    SEQ_T sequence;

    bool is_initalized[KEYS];
    WIN_T left_wid[KEYS];
    TIME_T max_timestamp[KEYS];
    WIN_T max_wid[KEYS];
    time_state_t<OP> states[N][KEYS];

    WIN_T curr_key;
    WIN_T curr_left_wid;
    TIME_T curr_max_timestamp;
    WIN_T curr_max_wid;
    time_state_t<OP> curr_states[N];


    _keyed_late_sliding_bucket_t()
    : sequence(0)
    , curr_key(-1)
    , curr_left_wid(0)
    , curr_max_timestamp(LATENESS)
    , curr_max_wid(N - 1)
    {
        #pragma HLS array_partition variable=is_initalized  type=complete
        #pragma HLS array_partition variable=left_wid       type=complete
        #pragma HLS array_partition variable=max_timestamp  type=complete
        #pragma HLS array_partition variable=max_wid        type=complete

        #pragma HLS bind_storage    variable=states         type=RAM_S2P  impl=BRAM
        #pragma HLS array_partition variable=states         type=complete dim=1

        #pragma HLS array_partition variable=curr_states    type=complete

        std::cout << "KEYS: " << KEYS << " SIZE: " << SIZE << " STEP: " << STEP << " LATENESS: " << LATENESS << " N: " << N << std::endl;
        KEYED_LATE_BUCKET_INIT:
        for (KEY_T k = 0; k < KEYS; ++k) {
        #pragma HLS UNROLL
            is_initalized[k] = false;
        }
    }

    template <typename STREAM_OUT>
    void _process(const KEY_T key, const IN_T in, const TIME_T timestamp, const bool valid, STREAM_OUT ostrms[N])
    {
    #pragma HLS INLINE
    #pragma HLS dependence variable=states type=intra direction=RAW false

        const WIN_T _left_wid = (timestamp < SIZE ? 0 : DIV_CEIL(timestamp - SIZE + 1, STEP));
        const WIN_T _right_wid = DIV_FLOOR(timestamp, STEP);

        if (curr_key != key) {
            if (curr_key != -1) {
                // store values for old key
                is_initalized[curr_key] = true;
                left_wid[curr_key]      = curr_left_wid;
                max_timestamp[curr_key] = curr_max_timestamp;
                max_wid[curr_key]       = curr_max_wid;

                PROCESS_STORE_STATES:
                for (WIN_T i = 0; i < N; ++i) {
                #pragma HLS UNROLL
                    states[i][curr_key] = curr_states[i];
                }
            }

            curr_key = key;

            const bool _is_initialized = is_initalized[key];
            curr_left_wid = (_is_initialized) ? left_wid[key] : 0;
            curr_max_timestamp = (_is_initialized) ? max_timestamp[key] : LATENESS;
            curr_max_wid = (_is_initialized) ? max_wid[key] : N - 1;

            PROCESS_INIT_LOAD_STATES:
            for (WIN_T i = 0; i < N; ++i) {
            #pragma HLS UNROLL
                curr_states[i].wid       = (_is_initialized) ? states[i][key].wid       : WIN_T(-1);
                curr_states[i].value     = (_is_initialized) ? states[i][key].value     : OP::identity();
                curr_states[i].timestamp = (_is_initialized) ? states[i][key].timestamp : TIME_T(-1);
            }
        }

        const bool _drop = !valid || (timestamp < curr_max_timestamp - LATENESS);
        const WIN_T old_left_wid = curr_left_wid;

        curr_left_wid = (_right_wid > curr_max_wid) ? (_right_wid - N + 1) : (curr_max_wid - N + 1);
        curr_max_wid = (_right_wid > curr_max_wid) ? _right_wid : curr_max_wid;
        curr_max_timestamp = (timestamp > curr_max_timestamp) ? timestamp : curr_max_timestamp;

        const WIN_T _left_widx = curr_left_wid % N;
        const WIN_T _base_wid = curr_left_wid - _left_widx;

        // print_array("curr_states", curr_states, N);


        SEND_RESULTS:
        for (WIN_T i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            const time_state_t<OP> state = curr_states[i];
            if (state.wid >= old_left_wid && state.wid < curr_left_wid) {
                // std::cout << "output: " << state.to_result_key(key, sequence) << std::endl;
                ostrms[i].write(state.to_result_key(key, sequence));
            }
        }
        sequence++;

        // std::cout << "key: " << key << " timestamp: " << timestamp << " left_wid: " << _left_wid << " right_wid: " << _right_wid << " drop: " << _drop << std::endl;

        if (!_drop) {
            UPDATE_STATE:
            for (WIN_T i = 0; i < N; ++i) {
            #pragma HLS UNROLL
                const time_state_t<OP> state = curr_states[i];
                const WIN_T _wid  = (i >= _left_widx ? curr_left_wid + i - _left_widx : curr_left_wid + N - _left_widx + i);
                const bool first_insert = (state.wid != _wid);
                const AGG_T agg = first_insert ? OP::identity() : state.value;

                if (_left_wid <= _wid && _wid <= _right_wid) {
                    curr_states[i].wid = _wid;
                    curr_states[i].value = OP::combine(agg, OP::lift(in));
                    curr_states[i].timestamp = first_insert ? timestamp : state.timestamp;
                    // curr_states[i].timestamp = first_insert ? timestamp : (state.timestamp < timestamp ? state.timestamp : timestamp);
                }
            }
        }
    }

    template <
        typename STREAM_IN,
        typename STREAM_VALID,
        typename STREAM_OUT,
        typename KEY_EXTRACTOR_T
    >
    void process(STREAM_IN & istrm, STREAM_VALID & vstrm, STREAM_OUT ostrms[N], KEY_EXTRACTOR_T && key_extractor)
    {
        using T_IN  = typename STREAM_IN::data_t;

        bool last = istrm.read_eos();
        TIME_BUCKET_WHILE:
        while (!last) {
        #pragma HLS PIPELINE II = L
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

            const T_IN in = istrm.read();
            auto key = key_extractor(in);
            bool valid = vstrm.read();

            last = istrm.read_eos();

            _process(key, in.value, in.timestamp, valid, ostrms);
        }

        TIME_BUCKET_EOS:
        for (WIN_T i = 0; i < N; ++i) {
            ostrms[i].write_eos();
        }
    }
};


template <
    typename OP,
    unsigned int SIZE = 1,
    unsigned int LATENESS = 0,
    typename STREAM_IN,
    typename STREAM_OUT
>
void TimeTumblingWindowOperator(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm
)
{
    static constexpr unsigned int N = (1 + (LATENESS + SIZE - 1) / SIZE);

    using IN_T = typename STREAM_IN::data_t;
    using STREAM_RESULT_T = fx::stream<time_result_t<OP>, N>;

    fx::stream<IN_T, N> _istrm("_istrm");
    fx::stream_single<bool, N> vstrm("vstrm");
    STREAM_RESULT_T result_strms[N];
    
    _late_bucket_t<OP, SIZE, LATENESS> bucket;

    #pragma HLS DATAFLOW
    send_and_flush<OP, 1>(istrm, _istrm, vstrm);
    bucket.process(_istrm, vstrm, result_strms);
    fx::SNtoS_LB<N>(result_strms, ostrm);
}

template <
    typename OP,
    unsigned int KEYS = 1,
    unsigned int SIZE = 1,
    unsigned int LATENESS = 0,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T
>
void KeyedTimeTumblingWindowOperator(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    KEY_EXTRACTOR_T && key_extractor
)
{
    static constexpr unsigned int N = (1 + (LATENESS + SIZE - 1) / SIZE);

    using KEY_T = unsigned int;
    using IN_T = typename STREAM_IN::data_t;
    using RESULT_T = keyed_time_result_t<OP, KEY_T>;


    // TODO: verificare che la depth di STREAM_RESULT_T sia corretta
    using STREAM_RESULT_T = fx::stream<RESULT_T, KEYS>; 

    fx::stream<IN_T, N> _istrm("_istrm");
    fx::stream_single<bool, N> vstrm("vstrm");
    STREAM_RESULT_T result_strms[N];
    
    _keyed_late_bucket_t<OP, KEYS, SIZE, LATENESS> bucket;

    #pragma HLS DATAFLOW
    send_and_flush<OP, KEYS>(istrm, _istrm, vstrm);
    bucket.process(_istrm, vstrm, result_strms, std::forward<KEY_EXTRACTOR_T>(key_extractor));
    fx::route_min_rec<N>(result_strms, ostrm,
        [](const RESULT_T & a, const RESULT_T & b) {
            return (a.sequence < b.sequence) || ((a.sequence == b.sequence) && (a.timestamp < b.timestamp));
        }
    );
}

template <
    typename OP,
    unsigned int KEYS = 1,
    unsigned int SIZE = 1,
    unsigned int STEP = 1,
    unsigned int LATENESS = 0,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T
>
void KeyedTimeSlidingWindowOperator(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    KEY_EXTRACTOR_T && key_extractor
)
{
    static constexpr unsigned int N = DIV_CEIL(SIZE + LATENESS, STEP);

    using KEY_T = unsigned int;
    using IN_T = typename STREAM_IN::data_t;
    using RESULT_T = keyed_time_result_t<OP, KEY_T>;

    // TODO: verificare che la depth di STREAM_RESULT_T sia corretta
    using STREAM_RESULT_T = fx::stream<RESULT_T, KEYS * 64>; 

    fx::stream<IN_T, 64> _istrm("_istrm");
    fx::stream_single<bool, 64> vstrm("vstrm");
    STREAM_RESULT_T result_strms[N];
    
    _keyed_late_sliding_bucket_t<OP, KEYS, SIZE, STEP, LATENESS> bucket;

    #pragma HLS DATAFLOW
    send_and_flush<OP, KEYS>(istrm, _istrm, vstrm);
    bucket.process(_istrm, vstrm, result_strms, std::forward<KEY_EXTRACTOR_T>(key_extractor));
    fx::route_min_rec<N>(result_strms, ostrm,
        [](const RESULT_T & a, const RESULT_T & b) {
            return (a.sequence < b.sequence) || ((a.sequence == b.sequence) && (a.timestamp < b.timestamp));
        }
    );
}

}

#endif // __WINDOW_HPP__
