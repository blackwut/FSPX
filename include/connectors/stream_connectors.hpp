#ifndef __STREAM_CONNECTORS_HPP__
#define __STREAM_CONNECTORS_HPP__


#include "ap_int.h"
#include "../common.hpp"
#include "../streams/stream.hpp"


namespace fx {

struct RoundRobin {};
struct LoadBalancer {};
struct KeyBy {};
struct Broadcast {};


template <typename T>
void StoS(
    fx::stream<T> & istrm,
    fx::stream<T> & ostrm
)
{
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

// template <int N, typename T>
// void StoSN(
//     hls::stream<T> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T> ostrms[N],
//     hls::stream<bool> e_ostrms[N],
//     RoundRobin policy
// )
// {
//     int id = 0;
//     bool last = e_istrm.read();

// StoSN:
//     while (!last) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         T t = istrm.read();

//         ostrms[id].write(t);
//         e_ostrms[id].write(false);

//         last = e_istrm.read();
//         id = (id + 1 == N) ? 0 : (id + 1);
//     }

//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         e_ostrms[i].write(true);
//     }
// }

template <int N, typename T>
void StoSN(
    fx::stream<T> & istrm,
    fx::stream<T> ostrms[N],
    RoundRobin policy
)
{
    UNUSED(policy);
    int id = 0;
    bool last = istrm.read_eos();

StoSN_RoundRobin:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        ostrms[id].write(t);

        id = ((id + 1) == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <int N, typename T>
void SNtoS(
    fx::stream<T> istrms[N],
    fx::stream<T> & ostrm,
    RoundRobin policy
)
{
    UNUSED(policy);
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all ones

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

// template <int N, typename T>
// void StoSN(
//     hls::stream<T> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T> ostrms[N],
//     hls::stream<bool> e_ostrms[N],
//     LoadBalancer policy
// )
// {
//     int id = 0;
//     bool last = e_istrm.read();

// StoSN:
//     while (!last) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         if (false == ostrms[id].full()) {
//             T t = istrm.read();

//             ostrms[id].write(t);
//             e_ostrms[id].write(false);

//             last = e_istrm.read();
//         }
//         id = (id + 1 == N) ? 0 : (id + 1);
//     }

//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         e_ostrms[i].write(true);
//     }
// }

template <int N, typename T>
void StoSN(
    fx::stream<T> & istrm,
    fx::stream<T> ostrms[N],
    LoadBalancer policy
)
{
    UNUSED(policy);
    int id = 0;
    bool last = istrm.read_eos();

StoSN_LoadBalancer:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[id].full()) {
            T t = istrm.read();
            ostrms[id].write(t);
            last = istrm.read_eos();
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <int N, typename T>
void SNtoS(
    fx::stream<T> istrms[N],
    fx::stream<T> & ostrm,
    LoadBalancer policy
)
{
    UNUSED(policy);
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all ones

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

// template <int N, typename T, typename T_KEY_EXTRACTOR>
// void StoSN(
//     hls::stream<T> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T> ostrms[N],
//     hls::stream<bool> e_ostrms[N],
//     KeyBy policy,
//     T_KEY_EXTRACTOR && key_extractor
// )
// {
//     bool last = e_istrm.read();

// StoSN:
//     while (!last) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         T t = istrm.read();
//         int key = key_extractor(t);

//         ostrms[key].write(t);
//         e_ostrms[key].write(false);

//         last = e_istrm.read();
//     }

//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         e_ostrms[i].write(true);
//     }
// }

template <int N, typename T, typename T_KEY_EXTRACTOR>
void StoSN(
    fx::stream<T> & istrm,
    fx::stream<T> ostrms[N],
    KeyBy policy,
    T_KEY_EXTRACTOR && key_extractor
)
{
    UNUSED(policy);
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

template <int N, typename T, typename T_KEY_GENERATOR>
void SNtoS(
    fx::stream<T> istrms[N],
    fx::stream<T> & ostrm,
    KeyBy policy,
    T_KEY_GENERATOR && key_generator
)
{
    UNUSED(policy);
    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all ones

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

// template <int N, typename T>
// void StoSN(
//     hls::stream<T> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T> ostrms[N],
//     hls::stream<bool> e_ostrms[N],
//     Broadcast policy
// )
// {
//     bool last = e_istrm.read();

// StoSN:
//     while (!last) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         T t = istrm.read();

//         for (int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             ostrms[i].write(t);
//             e_ostrms[i].write(false);
//         }

//         last = e_istrm.read();
//     }

//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         e_ostrms[i].write(true);
//     }
// }

template <int N, typename T>
void StoSN(
    fx::stream<T> & istrm,
    fx::stream<T> ostrms[N],
    Broadcast policy
)
{
    UNUSED(policy);
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

#endif // __STREAM_CONNECTORS_HPP__