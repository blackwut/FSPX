#ifndef __GENERIC_CONNECTORS_HPP__
#define __GENERIC_CONNECTORS_HPP__


#include "ap_int.h"
#include "../common.hpp"
#include "../streams/stream.hpp"

namespace fx {

template <
    typename STREAM_IN,
    typename STREAM_OUT
>
void StoS(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm
)
{
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        ostrm.write(t);
    }
    ostrm.write_eos();
}


//******************************************************************************
//
// Round Robin
//
//******************************************************************************

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void StoSN_RR(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N]
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    bool last = istrm.read_eos();

StoSN_RoundRobin:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        ostrms[id].write(t);

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_RR(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoS_RoundRobin:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            T t = istrms[id].read();
            lasts[id] = istrms[id].read_eos();

            ostrm.write(t);

            id = (id + 1 == N) ? 0 : (id + 1);
        }
    }

    ostrm.write_eos();
}


//******************************************************************************
//
// Load Balancer
//
//******************************************************************************

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void StoSN_LB(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N]
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    bool last = istrm.read_eos();

StoSN_LoadBalancer:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[id].full()) {
            T t = istrm.read();
            last = istrm.read_eos();

            ostrms[id].write(t);
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_LB(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoS_LoadBalancer:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = istrms[id].empty();

        if (!last && !empty) {
            T t = istrms[id].read();
            lasts[id] = istrms[id].read_eos();

            ostrm.write(t);
        }

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}


//******************************************************************************
//
// Key-By
//
//******************************************************************************

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T
>
void StoSN_KB(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N],
    KEY_EXTRACTOR_T && key_extractor
)
{
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_KeyBy:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        int key = key_extractor(t);
        ostrms[key].write(t);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_GENERATOR_T
>
void SNtoS_KB(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    KEY_GENERATOR_T && key_generator
)
{
    using T = typename STREAM_IN::data_t;

    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoS_KeyBy:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        id = key_generator(index);
        index++;

        if (!lasts[id]) {
            T t = istrms[id].read();
            lasts[id] = istrms[id].read_eos();

            ostrm.write(t);
        }
    }

    ostrm.write_eos();
}


//******************************************************************************
//
// Broadcast
//
//******************************************************************************

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void StoSN_B(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N]
)
{
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_Broadcast:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            ostrms[i].write(t);
        }
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

}

#endif // __GENERIC_CONNECTORS_HPP__