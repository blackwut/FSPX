#ifndef __CONNECTORS_ALL2ALL__
#define __CONNECTORS_ALL2ALL__

#include "../common.hpp"
#include "../streams/streams.hpp"
#include "generic.hpp"

namespace fx {
namespace A2A {


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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
#pragma HLS dataflow

Emitter_BR:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_B<M>(istrms[i], ostrms[i], "Emitter_BR");
    }
}


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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
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
// TODO: check if the following is correct!
#warning "FX: The following warning is expected!"
#pragma HLS dataflow
Collector_KB:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, "Collector_KB", key_generator);
    }
}

} // namespace A2A
} // namespace fx

#endif // __CONNECTORS_ALL2ALL__
