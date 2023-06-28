#ifndef __CONNECTORS_GENERIC_HPP__
#define __CONNECTORS_GENERIC_HPP__


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
    STREAM_OUT & ostrm,
    const char * name = ""
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

        #if defined(__DEBUG__CONNECTORS__)
        std::stringstream ss;
        ss << "StoS" << " (last: " << last << ")";
        print_debug(ss.str(), name, t);
        #endif

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
    STREAM_OUT ostrms[N],
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    bool last = istrm.read_eos();

StoSN_RR:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        #if defined(__DEBUG__CONNECTORS__)
        std::stringstream ss;
        ss << "StoSN_RR" << " (to: " << id << ", last: " << last << ")";
        print_debug(ss.str(), name, t);
        #endif

        ostrms[id].write(t);

        id = (id + 1 == N) ? 0 : (id + 1);
    }

StoSN_RR_EOS:
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
    STREAM_OUT & ostrm,
    const char * name = ""
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

SNtoS_RR:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            T t = istrms[id].read();
            lasts[id] = istrms[id].read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNtoS_RR" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);

            id = (id + 1 == N) ? 0 : (id + 1);
        }
        // TODO: this should be correct but in the following link the implementation is different
        // https://github.com/Xilinx/Vitis_Libraries/blob/2022.2/utils/L1/include/xf_utils_hw/stream_n_to_one/round_robin.hpp#L321
        //
        // Explaination: in the linked code the following line is inside the if statement
        // id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}

template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNMtoS_RR(
    STREAM_IN istrms[N][M],
    STREAM_OUT & ostrm,
    int m,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNMtoS_RR_INIT:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i][m].read_eos();
    }

SNMtoS_RR:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        if (!lasts[id]) {
            T t = istrms[id][m].read();
            lasts[id] = istrms[id][m].read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNMtoS_RR" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);

            // id = (id + 1 == N) ? 0 : (id + 1);
        }
        // TODO: this should be correct but in the following link the implementation is different
        // https://github.com/Xilinx/Vitis_Libraries/blob/2022.2/utils/L1/include/xf_utils_hw/stream_n_to_one/round_robin.hpp#L321
        //
        // Explaination: in the linked code the following line is inside the if statement
        id = (id + 1 == N) ? 0 : (id + 1);
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
    STREAM_OUT ostrms[N],
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    bool last = istrm.read_eos();

StoSN_LB:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[id].full()) {
            T t = istrm.read();
            last = istrm.read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "StoSN_LB" << " (to: " << id << ", last: " << last << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrms[id].write(t);
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }

StoSN_LB_EOS:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

#if 0
template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_LB(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    const char * name = ""
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

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNtoS_LB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}
#else
template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_LB(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNtoS_LB_INIT:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i].read_eos();
    }

SNtoS_LB:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = istrms[id].empty();

        if (!last && !empty) {
            T t = istrms[id].read();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNtoS_LB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }

        bool empty_eos = istrms[id].empty_eos();
        lasts[id] = (last || empty_eos) ? last : istrms[id].read_eos();

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}
#endif

template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void StoSNM_LB(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N][M],
    int n,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    bool last = istrm.read_eos();

StoSNM_LB:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[n][id].full()) {
            T t = istrm.read();
            last = istrm.read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "StoSNM_LB" << " (to: " << id << ", last: " << last << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrms[n][id].write(t);
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }

StoSNM_LB_EOS:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[n][i].write_eos();
    }
}

#if 0
template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNMtoS_LB(
    STREAM_IN istrms[N][M],
    STREAM_OUT & ostrm,
    int m,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNMtoS_LoadBalancer_Init:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i][m].read_eos();
    }

SNMtoS_LoadBalancer:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = istrms[id][m].empty();

        if (!last && !empty) {
            T t = istrms[id][m].read();
            lasts[id] = istrms[id][m].read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNMtoS_LB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}
#else
template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNMtoS_LB(
    STREAM_IN istrms[N][M],
    STREAM_OUT & ostrm,
    int m,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNMtoS_LB_INIT:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i][m].read_eos();
    }

SNMtoS_LB:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        bool last = lasts[id];
        bool empty = istrms[id][m].empty();

        if (!last && !empty) {
            T t = istrms[id][m].read();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNMtoS_LB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }

        bool empty_eos = istrms[id][m].empty_eos();
        lasts[id] = (last || empty_eos) ? last : istrms[id][m].read_eos();

        id = (id + 1 == N) ? 0 : (id + 1);
    }

    ostrm.write_eos();
}
#endif

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
    KEY_EXTRACTOR_T && key_extractor,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_KB:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        int key = key_extractor(t);
        ostrms[key].write(t);

        #if defined(__DEBUG__CONNECTORS__)
        std::stringstream ss;
        ss << "StoSN_KB" << " (to: " << key << ", last: " << last << ")";
        print_debug(ss.str(), name, t);
        #endif
    }

StoSN_KB_EOS:
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
    int m,
    KEY_GENERATOR_T && key_generator,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNtoS_KB_INIT:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i][m].read_eos();
    }

SNtoS_KB:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        id = key_generator(index);
        index++;

        if (!lasts[id]) {
            T t = istrms[id][m].read();
            lasts[id] = istrms[id][m].read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNtoS_KB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }
    }

    ostrm.write_eos();
}


template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_GENERATOR_T
>
void SNMtoS_KB(
    STREAM_IN istrms[N][M],
    STREAM_OUT & ostrm,
    int m,
    KEY_GENERATOR_T && key_generator,
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    int index = 0;
    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

SNMtoS_KB_INIT:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = istrms[i][m].read_eos();
    }

SNMtoS_KB:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        id = key_generator(index);
        index++;

        if (!lasts[id]) {
            T t = istrms[id][m].read();
            lasts[id] = istrms[id][m].read_eos();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNMtoS_KB" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

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
    STREAM_OUT ostrms[N],
    const char * name = ""
)
{
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_B:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        #if defined(__DEBUG__CONNECTORS__)
        std::stringstream ss;
        ss << "StoSN_B" << " (to: all" << ", last: " << last << ")";
        print_debug(ss.str(), name, t);
        #endif

    StoSN_B_WRITE:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            ostrms[i].write(t);
        }
    }

StoSN_B_EOS:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

}

#endif // __CONNECTORS_GENERIC_HPP__