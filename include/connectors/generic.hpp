#ifndef __CONNECTORS_GENERIC_HPP__
#define __CONNECTORS_GENERIC_HPP__

#include "ap_int.h"
#include "../common.hpp"
#include "../streams/stream.hpp"


namespace fx {

//******************************************************************************
//
// Single Stream
//
//******************************************************************************

template <
    typename stream_input_t,
    typename stream_output_t
>
void StoS (
    stream_input_t & stream_in,
    stream_output_t & stream_out
)
{
    using data_t = typename stream_input_t::data_t;

    bool last = stream_in.read_eos();

    StoS:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        const data_t t = stream_in.read();
        last = stream_in.read_eos();
        stream_out.write(t);
    }
    stream_out.write_eos();
}

template <
    typename stream_input_t,
    typename stream_output_t
>
void StoS_NB (
    stream_input_t & stream_in,
    stream_output_t & stream_out
)
{
    using data_t = typename stream_input_t::data_t;

    bool last = false;

    StoS_NB:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!stream_in.empty_eos()) {
            last = stream_in.read_eos();
        }

        if (!stream_in.empty()) {
            const data_t t = stream_in.read();
            stream_out.write(t);
        }
    }
    stream_out.write_eos();
}


//******************************************************************************
//
// Round Robin
//
//******************************************************************************

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t
>
void StoSN_RR (
    stream_input_t & stream_in,
    stream_output_t streams_out[N]
)
{
    using sid_t  = uint_for<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    bool last = stream_in.read_eos();

    StoSN_RR:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        const data_t t = stream_in.read();
        last = stream_in.read_eos();
        streams_out[id].write(t);

        increment<N>(id);
    }

    StoSN_RR_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL

        streams_out[i].write_eos();
    }
}

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t
>
void SNtoS_RR (
    stream_input_t streams_in[N],
    stream_output_t & stream_out
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNtoS_RR_FIRST_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        lasts[i] = streams_in[i].read_eos();
    }

    SNtoS_RR:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            const data_t t = streams_in[id].read();
            lasts[id] = streams_in[id].read_eos();
            stream_out.write(t);
        }
        
        increment<N>(id);
    }

    stream_out.write_eos();
}

template <
    unsigned int N,
    unsigned int M,
    typename stream_input_t,
    typename stream_output_t
>
void SNMtoS_RR (
    stream_input_t streams_in[N][M],
    stream_output_t & stream_out,
    const unsigned int m
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNMtoS_RR_FIRST_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        lasts[i] = streams_in[i][m].read_eos();
    }

    SNMtoS_RR:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            const data_t t = streams_in[id][m].read();
            lasts[id] = streams_in[id][m].read_eos();
            stream_out.write(t);
        }

        increment<N>(id);
    }

    stream_out.write_eos();
}


//******************************************************************************
//
// Load Balancer
//
//******************************************************************************

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t
>
void StoSN_LB (
    stream_input_t & stream_in,
    stream_output_t streams_out[N]
)
{
    using sid_t = uint_for<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    bool last = stream_in.read_eos();

    StoSN_LB:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        if (!streams_out[id].full()) {
            const data_t t = stream_in.read();
            last = stream_in.read_eos();
            streams_out[id].write(t);
        }
        increment<N>(id);
    }

    StoSN_LB_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL

        streams_out[i].write_eos();
    }
}

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t
>
void SNtoS_LB (
    stream_input_t streams_in[N],
    stream_output_t & stream_out
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNtoS_LB:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = streams_in[id].empty();

        if (!last && !empty) {
            const data_t t = streams_in[id].read();
            stream_out.write(t);
        }

        const bool empty_eos = streams_in[id].empty_eos();
        lasts[id] = (last || empty_eos) ? last : streams_in[id].read_eos();
        increment<N>(id);
    }

    stream_out.write_eos();
}

template <
    typename stream_input_t,
    typename stream_output_t,
    typename comparator_t
>
void route_min (
    stream_input_t streams_in[2],
    stream_output_t & stream_out,
    comparator_t && comparator
)
{
    #pragma HLS INLINE OFF
    
    using data_t = typename stream_input_t::data_t;
    using mask_t = ap_uint<2>;

    mask_t mask = 0;
    data_t buffer[2];

    ROUTE_MIN_BUFFER_INIT:
    for (unsigned int i = 0; i < 2; ++i) {
        #pragma HLS UNROLL
        
        buffer[i] = data_t();
    }

    mask_t lasts = 0;
    const mask_t ends = ~lasts;

    ROUTE_MIN:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
    
        ROUTE_MIN_UPDATE_BUFFER:
        for (unsigned int i = 0; i < 2; ++i) {
            #pragma HLS UNROLL
            
            if (mask[i] == 0) {
                if (!streams_in[i].empty()) {
                    buffer[i] = streams_in[i].read();
                    mask[i] = 1;
                }

                if (!streams_in[i].empty_eos()) {
                    lasts[i] = streams_in[i].read_eos();
                }
            }
        }

        if (mask[0] && mask[1]) {
            if (comparator(buffer[0], buffer[1])) {
                stream_out.write(buffer[0]);
                mask[0] = 0;
            } else {
                stream_out.write(buffer[1]);
                mask[1] = 0;
            }
        } else if (mask[0]) {
            stream_out.write(buffer[0]);
            mask[0] = 0;
        } else if (mask[1]) {
            stream_out.write(buffer[1]);
            mask[1] = 0;
        }
    }

    stream_out.write_eos();
}

// Base case for N = 1
template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t,
    typename comparator_t
>
typename std::enable_if<N == 1, void>::type
route_min_rec (
    stream_input_t streams_in[1],
    stream_output_t & stream_out,
    comparator_t && comparator
)
{
    #pragma HLS INLINE
    UNUSED(comparator);
    StoS_NB(streams_in[0], stream_out);
}

// Specialization for N = 2
template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t,
    typename comparator_t
>
typename std::enable_if<N == 2, void>::type
route_min_rec (
    stream_input_t streams_in[2],
    stream_output_t & stream_out,
    comparator_t && comparator
)
{
    #pragma HLS INLINE
    route_min(streams_in, stream_out, std::forward<comparator_t>(comparator));
}

// Specialization for N > 2
template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t,
    typename comparator_t
>
typename std::enable_if<(N > 2), void>::type
route_min_rec (
    stream_input_t streams_in[N],
    stream_output_t & stream_out,
    comparator_t && comparator
)
{
    using data_t = typename stream_input_t::data_t;
    using stream_intern_t = fx::stream<data_t, 16>; // TODO: fix the depth

    static constexpr int M = N / 2;
    static constexpr int RES = N % 2;
    static constexpr int M_RES = M + RES;

    stream_intern_t internal_streams[M_RES];

    for (unsigned int i = 0; i < M; ++i) {
        #pragma HLS UNROLL
        route_min(streams_in + i * 2, internal_streams[i], std::forward<comparator_t>(comparator));
    }

    if (RES == 1) {
        StoS_NB(streams_in[M * 2], internal_streams[M]);
    }

    route_min_rec<M_RES>(internal_streams, stream_out, std::forward<comparator_t>(comparator));
}

template <
    unsigned int N,
    unsigned int M,
    typename stream_input_t,
    typename stream_output_t
>
void StoSNM_LB (
    stream_input_t & stream_in,
    stream_output_t streams_out[N][M],
    unsigned int n
)
{
    using sid_t = uint_for<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    bool last = stream_in.read_eos();

    StoSNM_LB:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        if (!streams_out[n][id].full()) {
            const data_t t = stream_in.read();
            last = stream_in.read_eos();
            streams_out[n][id].write(t);
        }
        increment<N>(id);
    }

    StoSNM_LB_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        streams_out[n][i].write_eos();
    }
}

template <
    unsigned int N,
    unsigned int M,
    typename stream_input_t,
    typename stream_output_t
>
void SNMtoS_LB (
    stream_input_t streams_in[N][M],
    stream_output_t & stream_out,
    unsigned int m
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    sid_t id = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNMtoS_LB:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = streams_in[id][m].empty();

        if (!last && !empty) {
            const data_t t = streams_in[id][m].read();
            stream_out.write(t);
        }

        const bool empty_eos = streams_in[id][m].empty_eos();
        lasts[id] = (last || empty_eos) ? last : streams_in[id][m].read_eos();
        increment<N>(id);
    }

    stream_out.write_eos();
}


//******************************************************************************
//
// Key-By
//
//******************************************************************************

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t,
    typename key_extractor_t
>
void StoSN_KB (
    stream_input_t & stream_in,
    stream_output_t streams_out[N],
    key_extractor_t && key_extractor
)
{
    using key_t = uint_for<N>;
    using data_t = typename stream_input_t::data_t;

    bool last = stream_in.read_eos();

    StoSN_KB:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        const data_t t = stream_in.read();
        last = stream_in.read_eos();
        const key_t key = key_extractor(t) % N;
        streams_out[key].write(t);
    }

    StoSN_KB_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL

        streams_out[i].write_eos();
    }
}

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t,
    typename key_generator_t
>
void SNtoS_KB (
    stream_input_t streams_in[N],
    stream_output_t & stream_out,
    unsigned int m,
    key_generator_t && key_generator
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    index_t index = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNtoS_KB_FIRST_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL

        lasts[i] = streams_in[i][m].read_eos();
    }

    SNtoS_KB:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        const sid_t id = sid_t(key_generator(index)); // TODO: check if its ok to use sid_t
        index++;

        if (!lasts[id]) {
            const data_t t = streams_in[id][m].read();
            lasts[id] = streams_in[id][m].read_eos();
            stream_out.write(t);
        }
    }

    stream_out.write_eos();
}


template <
    unsigned int N,
    unsigned int M,
    typename stream_input_t,
    typename stream_output_t,
    typename key_generator_t
>
void SNMtoS_KB (
    stream_input_t streams_in[N][M],
    stream_output_t & stream_out,
    unsigned int m,
    key_generator_t && key_generator
)
{
    using sid_t = uint_for<N>;
    using last_t = ap_uint<N>;
    using data_t = typename stream_input_t::data_t;

    index_t index = 0;
    last_t lasts = 0;
    const last_t ends = ~lasts;

    SNMtoS_KB_FIRST_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL

        lasts[i] = streams_in[i][m].read_eos();
    }

    SNMtoS_KB:
    while (lasts != ends) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        const sid_t id = key_generator(index);
        index++;

        if (!lasts[id]) {
            const data_t t = streams_in[id][m].read();
            lasts[id] = streams_in[id][m].read_eos();
            stream_out.write(t);
        }
    }

    stream_out.write_eos();
}


//******************************************************************************
//
// Broadcast
//
//******************************************************************************

template <
    unsigned int N,
    typename stream_input_t,
    typename stream_output_t
>
void StoSN_BR (
    stream_input_t & stream_in,
    stream_output_t streams_out[N]
)
{
    using data_t = typename stream_input_t::data_t;

    bool last = stream_in.read_eos();

    StoSN_BR:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        const data_t t = stream_in.read();
        last = stream_in.read_eos();

        StoSN_BR_WRITE:
        for (unsigned int i = 0; i < N; ++i) {
            #pragma HLS UNROLL
            
            streams_out[i].write(t);
        }
    }

    StoSN_BR_EOS:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        streams_out[i].write_eos();
    }
}

}

#endif // __CONNECTORS_GENERIC_HPP__
