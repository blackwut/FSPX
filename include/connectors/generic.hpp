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
    UNUSED(name);
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

template <
    int N,
    typename STREAM_IN
>
void debug_streams(
    STREAM_IN istrms[N],
    STREAM_IN ostrms[N],
    const char * name = ""
)
{
#if !defined(__SYNTHESIS__)
    UNUSED(name);
    using T = typename STREAM_IN::data_t;

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        bool last = istrms[i].read_eos();

        std::cout << "debug_streams(" << i << ")" << std::endl;
    
        debug_streams:
        while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
            T t = istrms[i].read();
            last = istrms[i].read_eos();

            std::cout << t << std::endl;

            ostrms[i].write(t);
        }
        ostrms[i].write_eos();
    }
#endif
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
    UNUSED(name);
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
    UNUSED(name);
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
        }

        id = (id + 1 == N) ? 0 : (id + 1);
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
    UNUSED(name);
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
        }

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
    UNUSED(name);
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
    UNUSED(name);
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

// SNtoS_LB_INIT:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         lasts[i] = istrms[i].read_eos();
//         std::cout << "SN.last[" << i << "]: " << lasts[i] << std::endl;
//     }

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


template <int N>
void print_bits(const ap_uint<N> val, const std::string message)
{
    std::cout << message << ": ";
    for (int i = N - 1; i >= 0; --i) {
        std::cout << val[i];
    }
    std::cout << std::endl;
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_LB_check(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    const char * name = ""
)
{
    HW_STATIC_ASSERT(N <= 32, "N must be less than or equal to 32");

    UNUSED(name);
    using T = typename STREAM_IN::data_t;
    using ID_T = ap_uint<LOG2_CEIL(N)>;
    using LAST_T = ap_uint<N>;
    using MASK_T = ap_uint<N + 1>;

    static MASK_T ZERO_MASK[N + 1];
    INIT_ZERO_MASK:
    for (int i = 0; i < N + 1; ++i) {
    #pragma HLS UNROLL
        ZERO_MASK[i] = ~MASK_T(0) << i;
    }

    #pragma HLS array_partition variable=ZERO_MASK complete dim=1
    #pragma HLS bind_storage variable=ZERO_MASK type=rom_1p impl=lutram

    ID_T id = N - 1;
    LAST_T lasts = 0;
    const LAST_T ends = ~lasts;   // set all bits to one

SNtoS_LB_check:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

    // #if !__SYNTHESIS__
    //     std::cout << "id: " << id << std::endl;
    // #endif

        MASK_T mask = ~MASK_T(0);
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            mask[i] = !istrms[i].empty_eos();
        }

        const MASK_T low_mask = mask;
        const MASK_T high_mask = (mask & ZERO_MASK[id + 1]);
        const ID_T low_id = __builtin_ctz(low_mask);
        const ID_T high_id = __builtin_ctz(high_mask);

        if (high_mask << 1) {
            id = high_id;
        } else {
            id = low_id;
        }

    // #if !__SYNTHESIS__
    //     print_bits(mask, "M");
    //     print_bits(ZERO_MASK[id], "Z");
    //     print_bits(high_mask, "H");
    //     print_bits(low_mask, "L");
    //     std::cout << "high_id: " << high_id << std::endl;
    //     std::cout << "low_id: "  << low_id << std::endl;
    //     std::cout << "id: " << id << std::endl;
    // #endif

        const bool empty = istrms[id].empty();
        if (!empty) {
            const T t = istrms[id].read();

            #if defined(__DEBUG__CONNECTORS__)
            std::stringstream ss;
            ss << "SNtoS_LB_check" << " (from: " << id << ", last: " << lasts[id] << ")";
            print_debug(ss.str(), name, t);
            #endif

            ostrm.write(t);
        }

        lasts[id] = istrms[id].read_eos();
    }

    ostrm.write_eos();
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_LB_recursive(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    const char * name = ""
)
{
#pragma HLS INLINE
    using DATA_T = typename STREAM_IN::data_t;
    static constexpr int MAX_N = 16;

    static constexpr int M = N / MAX_N;
    static constexpr int RES = N % MAX_N;
    static constexpr int M_RES = M + (RES > 0);

    if constexpr (N == 1) {
        StoS(istrms[0], ostrm, name);
    } else if constexpr (N <= MAX_N) {
        SNtoS_LB_check<N>(istrms, ostrm, name);
    } else {
        // N > 16
        fx::stream<DATA_T, M_RES> ostrms[M_RES];

        for (int i = 0; i < M; ++i) {
        #pragma HLS unroll
            SNtoS_LB_check<MAX_N>(istrms + i * MAX_N, ostrms[i], name);
        }
 
        if constexpr (RES == 1) {
            StoS(istrms[M * MAX_N], ostrms[M], name);
        } else if constexpr (RES > 1) {
            SNtoS_LB_check<RES>(istrms + M * MAX_N, ostrms[M], name);
        }
        SNtoS_LB_recursive<M_RES>(ostrms, ostrm, name);
    }
}

template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT
>
void SNtoS_Min(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    const char * name = ""
)
{
    UNUSED(name);
    using T = typename STREAM_IN::data_t;
    using ID_T = ap_uint<LOG2_CEIL(N)>;
    using MASK_T = ap_uint<N>;

    MASK_T buffer_mask = 0;
    T buffer[N];

    INIT_BUFFER:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        buffer[i] = T(0, 0, 0, -1);
    }

    MASK_T lasts = 0;
    const MASK_T ends = ~lasts;   // set all bits to one

    SNtoS_Min:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1

        UPDATE_BUFFER:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            if (buffer_mask[i] == 0) {
                if (!istrms[i].empty()) {
                    buffer[i] = istrms[i].read();
                    buffer_mask[i] = 1;
                }

                if (!istrms[i].empty_eos()) {
                    lasts[i] = istrms[i].read_eos();
                }
            }
        }

        // find the minimum
        ID_T id = 0;
        T min = buffer[0];
        FIND_MIN:
        for (int i = 1; i < N; ++i) {
        #pragma HLS UNROLL
            if (buffer_mask[i] && buffer[i] < min) {
                id = i;
                min = buffer[i];
            }
        }
        
        if (buffer_mask[id]) {
            ostrm.write(min);
            buffer[id] = T(0, 0, 0, -1);
            buffer_mask[id] = 0;
        }
    }

    ostrm.write_eos();
}


template <
    typename STREAM_IN,
    typename STREAM_OUT,
    typename COMPARATOR
>
void route_min(
    STREAM_IN istrms[2],
    STREAM_OUT & ostrm,
    COMPARATOR && comparator
)
{
#pragma HLS INLINE OFF
    using T = typename STREAM_IN::data_t;
    using MASK_T = ap_uint<2>;

    MASK_T buffer_mask = 0;
    T buffer[2];
    for (int i = 0; i < 2; ++i) {
    #pragma HLS UNROLL
        buffer[i].reset();
    }

    MASK_T lasts = 0;
    const MASK_T ends = ~lasts;

    ROUTE_MIN_LOOP:
    while (lasts != ends) {
    #pragma HLS PIPELINE II = 1
    
        // update buffer
        UPDATE_BUFFER:
        for (int i = 0; i < 2; ++i) {
        #pragma HLS UNROLL
            if (buffer_mask[i] == 0) {
                if (!istrms[i].empty()) {
                    buffer[i] = istrms[i].read();
                    buffer_mask[i] = 1;
                }

                if (!istrms[i].empty_eos()) {
                    lasts[i] = istrms[i].read_eos();
                }
            }
        }

        // find the minimum
        if (buffer_mask[0] && buffer_mask[1]) {
            if (comparator(buffer[0], buffer[1])) {
                ostrm.write(buffer[0]);
                buffer_mask[0] = 0;
            } else {
                ostrm.write(buffer[1]);
                buffer_mask[1] = 0;
            }
        } else if (buffer_mask[0]) {
            ostrm.write(buffer[0]);
            buffer_mask[0] = 0;
        } else if (buffer_mask[1]) {
            ostrm.write(buffer[1]);
            buffer_mask[1] = 0;
        }
    }

    ostrm.write_eos();
}


// template <
//     int N,
//     typename STREAM_IN,
//     typename STREAM_OUT
// >
// void route_min_rec(
//     STREAM_IN istrms[N],
//     STREAM_OUT & ostrm
// )
// {
// #pragma HLS INLINE
//     using T = typename STREAM_IN::data_t;

//     using STREAM_INTERN = fx::stream<T, 2>;

//     if constexpr (N == 1) {
//         StoS(istrms[0], ostrm);
//     } else if constexpr (N == 2) {
//         route_min(istrms, ostrm);
//     } else if constexpr (N > 2) {
        
//         static constexpr int M = N / 2;
//         static constexpr int RES = N % 2;
//         static constexpr int M_RES = M + (RES > 0);

//         STREAM_INTERN ostrms[M_RES];

//         for (int i = 0; i < M; ++i) {
//         #pragma HLS UNROLL
//             route_min(istrms + i * 2, ostrms[i]);
//         }

//         if constexpr (RES == 1) {
//             StoS(istrms[M * 2], ostrms[M]);
//         }

//         route_min_rec<M_RES>(ostrms, ostrm);
//     }
// }


// Base case for N = 1
template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename COMPARATOR
>
typename std::enable_if<N == 1, void>::type
route_min_rec(
    STREAM_IN istrms[1],
    STREAM_OUT & ostrm,
    COMPARATOR && comparator
)
{
    #pragma HLS INLINE
    StoS(istrms[0], ostrm);
}

// Specialization for N = 2
template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename COMPARATOR
>
typename std::enable_if<N == 2, void>::type
route_min_rec(
    STREAM_IN istrms[2],
    STREAM_OUT & ostrm,
    COMPARATOR && comparator
)
{
    #pragma HLS INLINE
    route_min(istrms, ostrm, std::forward<COMPARATOR>(comparator));
}

// General case for N > 2
template <
    int N,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename COMPARATOR
>
typename std::enable_if<(N > 2), void>::type
route_min_rec(
    STREAM_IN istrms[N],
    STREAM_OUT & ostrm,
    COMPARATOR && comparator
)
{
    #pragma HLS INLINE
    using T = typename STREAM_IN::data_t;
    using STREAM_INTERN = fx::stream<T, 2>;

    static constexpr int M = N / 2;
    static constexpr int RES = N % 2;
    static constexpr int M_RES = M + (RES > 0);

    STREAM_INTERN ostrms[M_RES];

    for (int i = 0; i < M; ++i) {
        #pragma HLS UNROLL
        route_min(istrms + i * 2, ostrms[i], std::forward<COMPARATOR>(comparator));
    }

    if (RES == 1) {
        StoS(istrms[M * 2], ostrms[M]);
    }

    route_min_rec<M_RES>(ostrms, ostrm, std::forward<COMPARATOR>(comparator));
}

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
    UNUSED(name);
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
    UNUSED(name);
    using T = typename STREAM_IN::data_t;

    int id = 0;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;   // set all bits to one

// SNMtoS_LB_INIT:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS UNROLL
//         lasts[i] = istrms[i][m].read_eos();
//         std::cout << "SNM.last[" << i << "]: " << lasts[i] << std::endl;
//     }

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
    UNUSED(name);
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_KB:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();
        int key = key_extractor(t) % N;
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
    UNUSED(name);
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
    UNUSED(name);
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
void StoSN_BR(
    STREAM_IN & istrm,
    STREAM_OUT ostrms[N],
    const char * name = ""
)
{
    UNUSED(name);
    using T = typename STREAM_IN::data_t;

    bool last = istrm.read_eos();

StoSN_BR:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        last = istrm.read_eos();

        #if defined(__DEBUG__CONNECTORS__)
        std::stringstream ss;
        ss << "StoSN_BR" << " (to: all" << ", last: " << last << ")";
        print_debug(ss.str(), name, t);
        #endif

    StoSN_BR_WRITE:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            ostrms[i].write(t);
        }
    }

StoSN_BR_EOS:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        ostrms[i].write_eos();
    }
}

}

#endif // __CONNECTORS_GENERIC_HPP__
