#ifndef __CONNECTORS_ALL2ALL__
#define __CONNECTORS_ALL2ALL__

#include "../common.hpp"
#include "../streams/streams.hpp"
#include "generic.hpp"


#define __SINGLE_FUNCTION__ 0

namespace fx {
namespace A2A {

#if __SINGLE_FUNCTION__

#include <algorithm>
#include <type_traits>

struct RR{};
struct LB{};
struct KB{};
struct BR{};

//******************************************************************************
//
// Emitter
//
//******************************************************************************

template <
    typename POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T
>
void Emitter(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M],
    KEY_EXTRACTOR_T && key_extractor = nullptr
)
{
    static_assert(
        std::all_of(
            {
                std::is_same<POLICY_T, RR>::value,
                std::is_same<POLICY_T, LB>::value,
                std::is_same<POLICY_T, KB>::value,
                std::is_same<POLICY_T, BR>::value,
            }
        ),
        "FX: Only RR, LB, KB and BR are supported policies!"
    );

#pragma HLS dataflow

Emitter:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        if (std::is_same<POLICY_T, RR>::value) {
            fx::StoSN_RR<M>(istrms[i], ostrms[i], "Emitter_RR");
        } else if (std::is_same<POLICY_T, LB>::value) {
            fx::StoSN_LB<M>(istrms[i], ostrms[i], "Emitter_LB");
        } else if (std::is_same<POLICY_T, KB>::value) {
            fx::StoSN_KB<M>(istrms[i], ostrms[i], key_extractor, "Emitter_KB");
        } else if (std::is_same<POLICY_T, BR>::value) {
            fx::StoSN_B<M>(istrms[i], ostrms[i], "Emitter_BR");
        }
    }
}

#else


template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Emitter_RR(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M]
)
{
#pragma HLS dataflow

Emitter_RR:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_RR<M>(istrms[i], ostrms[i], "Emitter_RR");
    }
}


template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Emitter_LB(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M]
)
{
#pragma HLS dataflow

Emitter_LB:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_LB<M>(istrms[i], ostrms[i], "Emitter_LB");
    }
}


template <
    int N,
    int M,
    typename KEY_EXTRACTOR_T,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Emitter_KB(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M],
    KEY_EXTRACTOR_T && key_extractor
)
{
#pragma HLS dataflow

Emitter_KB:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_KB<M>(istrms[i], ostrms[i], key_extractor, "Emitter_KB");
    }
}


template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Emitter_BR(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M]
)
{
#pragma HLS dataflow

Emitter_BR:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_B<M>(istrms[i], ostrms[i], "Emitter_BR");
    }
}
#endif


//******************************************************************************
//
// Collector
//
//******************************************************************************


#if __SINGLE_FUNCTION__

template <
    typename POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_GENERATOR_T
>
void Collector(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M],
    KEY_GENERATOR_T && key_generator = nullptr
)
{
    static_assert(
        std::all_of(
            {
                std::is_same<POLICY_T, RR>::value,
                std::is_same<POLICY_T, LB>::value,
                std::is_same<POLICY_T, KB>::value
            }
        ),
        "FX: Only RR, LB and KB are supported policies!"
    );

#pragma HLS dataflow
Collector:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        if (std::is_same<POLICY_T, RR>::value) {
            fx::SNMtoS_RR<N, M>(istrms, ostrms[i], i, "Collector_RR");
        } else if (std::is_same<POLICY_T, LB>::value) {
            fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "Collector_LB");
        } else if (std::is_same<POLICY_T, KB>::value) {
            fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, key_generator, "Collector_KB");
        }
    }
}

#else
template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Collector_RR(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M]
)
{
#pragma HLS dataflow
Collector_RR:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_RR<N, M>(istrms, ostrms[i], i, "Collector_RR");
    }
}


template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Collector_LB(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M]
)
{
#pragma HLS dataflow
Collector_LB:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "Collector_LB");
    }
}


template <
    int N,
    int M,
    typename KEY_GENERATOR_T,
    typename STREAM_IN,
    typename STREAM_OUT
>
void Collector_KB(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M],
    KEY_GENERATOR_T && key_generator
)
{
#pragma HLS dataflow
Collector_KB:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, "Collector_KB", key_generator);
    }
}

#endif


//******************************************************************************
//
// Map
//
//******************************************************************************

// TODO: restructure the whole library to implement everything as functors and try to use the following example to solve the problem of using templates to generate multiple fuctions based on the input and output policy_t
// struct RR {};

// template <typename POLICY_T = int, typename T>
// void keyby(T && fun)
// {
//     std::cout << "keyby" << std::endl;

//     if constexpr (std::is_same<POLICY_T, RR>::value) {
//         fun();
//     }
// }

// int main() {

//     // GetNameOfList<A>();
//     my_funct()(1);


//   	keyby([](){ std::cout << "lambda" << std::endl; });
//     //keyby<RR>([](){ std::cout << "lambda" << std::endl; });

//     return 0;
// }


} // namespace A2A
} // namespace fx

#endif // __CONNECTORS_ALL2ALL__
