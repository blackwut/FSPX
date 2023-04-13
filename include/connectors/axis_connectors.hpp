#ifndef __AXIS_CONNECTORS_HPP__
#define __AXIS_CONNECTORS_HPP__


#include "ap_int.h"
#include "../common.hpp"
#include "../streams/stream.hpp"
#include "../streams/axis_stream.hpp"


// Base
// Emitter      AtoSN V
// Splitter     StoAN V

// Merger       ANtoS V
// Collector    SNtoA V


// Advanced
// IRouter      ANtoSM  // Merger + Emitter
// ORouter      SNtoAM  // Collector + Splitter


namespace fx {

template <typename T, int DEPTH_IN, int DEPTH_OUT>
void AtoA(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> & ostrm
)
{
    bool last = false;
    T t = istrm.read(last);

AtoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        ostrm.write(t);
        t = istrm.read(last);
    }
    ostrm.write_eos();
}

template <typename T, int DEPTH_IN, int DEPTH_OUT>
void StoA(
    fx::stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> & ostrm
)
{
    bool last = istrm.read_eos();
StoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();
        ostrm.write(t);
    }
    ostrm.write_eos();
}

template <typename T, int DEPTH_IN, int DEPTH_OUT>
void AtoS(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::stream<T, DEPTH_OUT> & ostrm
)
{
    bool last = false;
    T t = istrm.read(last);

AtoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        ostrm.write(t);
        t = istrm.read(last);
    }
    ostrm.write_eos();
}


//******************************************************************************
//
// Round Robin
//
//******************************************************************************

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void StoAN_RR(
    fx::stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> ostrms[N]
)
{
    int id = 0;
    bool last = istrm.read_eos();

StoAN_RoundRobin:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void AtoSN_RR(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::stream<T, DEPTH_OUT> ostrms[N]
)
{
    int id = 0;
    bool last = false;
    T t = istrm.read(last);

AtoSN_RoundRobin:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        ostrms[id].write(t);
        t = istrm.read(last);

        id = ((id + 1) == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void ANtoS_RR(
    fx::axis_stream<T, DEPTH_IN> istrms[N],
    fx::stream<T, DEPTH_OUT> & ostrm
)
{
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    T ts[N];
    #pragma HLS array_partition variable=ts type=complete dim=1

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        bool last = false;
        ts[i] = istrms[i].read(last);
        lasts[i] = last;
    }

ANtoS_RoundRobin:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            ostrm.write(ts[id]);
            bool last = false;
            ts[id] = istrms[id].read(last);
            lasts[id] = last;

            id = (id + 1 == N) ? 0 : (id + 1);
        }
    }

    ostrm.write_eos();
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void SNtoA_RR(
    fx::stream<T, DEPTH_IN> istrms[N],
    fx::axis_stream<T, DEPTH_OUT> & ostrm
)
{
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoA_RoundRobin:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void StoAN_LB(
    fx::stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> ostrms[N]
)
{
    int id = 0;
    bool last = istrm.read_eos();

StoAN_LoadBalancer:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void AtoSN_LB(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::stream<T, DEPTH_OUT> ostrms[N]
)
{
    int id = 0;
    bool last = false;
    T t = istrm.read(last);

AtoSN_LoadBalancer:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[id].full()) {
            ostrms[id].write(t);
            t = istrm.read(last);
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void ANtoS_LB(
    fx::axis_stream<T, DEPTH_IN> istrms[N],
    fx::stream<T, DEPTH_OUT> & ostrm
)
{
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    T ts[N];
    #pragma HLS array_partition variable=ts type=complete dim=1

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        bool last = false;
        ts[i] = istrms[i].read(last);
        lasts[i] = last;
    }

ANtoS_LoadBalancer:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = istrms[id].empty();

        if (!last && !empty) {
            ostrm.write(ts[id]);

            bool last = false;
            ts[id] = istrms[id].read(last);
            lasts[id] = last;
        }

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void SNtoA_LB(
    fx::stream<T, DEPTH_IN> istrms[N],
    fx::axis_stream<T, DEPTH_OUT> & ostrm
)
{
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoA_LoadBalancer:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT, typename KEY_EXTRACTOR_T>
void StoAN_KB(
    fx::stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> ostrms[N],
    KEY_EXTRACTOR_T && key_extractor
)
{
    bool last = istrm.read_eos();

StoAN_KeyBy:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT, typename KEY_EXTRACTOR_T>
void AtoSN_KB(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::stream<T, DEPTH_OUT> ostrms[N],
    KEY_EXTRACTOR_T && key_extractor
)
{
    bool last = false;
    T t = istrm.read(last);

AtoSN_KeyBy:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        int key = key_extractor(t);
        ostrms[key].write(t);

        t = istrm.read(last);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT, typename T_KEY_GENERATOR>
void ANtoS_KB(
    fx::axis_stream<T, DEPTH_IN> istrms[N],
    fx::stream<T, DEPTH_OUT> & ostrm,
    T_KEY_GENERATOR && key_generator
)
{
    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    T ts[N];
    #pragma HLS array_partition variable=ts type=complete dim=1

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        bool last = false;
        ts[i] = istrms[i].read(last);
        lasts[i] = last;
    }

ANtoS_KeyBy:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        id = key_generator(index);
        index++;

        if (!lasts[id]) {
            ostrm.write(ts[id]);

            bool last = false;
            ts[id] = istrms[id].read(last);
            lasts[id] = last;
        }
    }

    ostrm.write_eos();
}

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT, typename T_KEY_GENERATOR>
void SNtoA_KB(
    fx::stream<T, DEPTH_IN> istrms[N],
    fx::axis_stream<T, DEPTH_OUT> & ostrm,
    T_KEY_GENERATOR && key_generator
)
{
    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoA_KeyBy:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void StoAN_B(
    fx::stream<T, DEPTH_IN> & istrm,
    fx::axis_stream<T, DEPTH_OUT> ostrms[N]
)
{
    bool last = istrm.read_eos();

StoAN_Broadcast:
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

template <int N, typename T, int DEPTH_IN, int DEPTH_OUT>
void AtoSN_B(
    fx::axis_stream<T, DEPTH_IN> & istrm,
    fx::stream<T, DEPTH_OUT> ostrms[N]
)
{
    bool last = false;
    T t = istrm.read(last);

AtoSN_Broadcast:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            ostrms[i].write(t);
        }

        t = istrm.read(last);
    }

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

}

#endif // __AXIS_CONNECTORS_HPP__