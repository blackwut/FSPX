#ifndef __CONNECTORS_ALL2ALL__
#define __CONNECTORS_ALL2ALL__

#include <utility>
#include <type_traits>
#include "../common.hpp"
#include "../streams/streams.hpp"
#include "generic.hpp"
#include "../operators/operators.hpp"


#define USE_CONSTEXPR_IF_IMPLEMENTATION 1

namespace fx {
namespace A2A {

enum Policy_t {
    RR,
    LB,
    KB,
    BR
};

// TODO: Add Generator and Drainer operators and replication functions for them
// TODO: Add implementation without constexpr if for operators

enum Operator_t {
    MAP,
    FILTER,
    FLATMAP,
    GENERATOR,
    DRAINER
};


//******************************************************************************
//
// Emitter
//
//******************************************************************************

#if USE_CONSTEXPR_IF_IMPLEMENTATION

template <
    Policy_t POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T = int
>
void Emitter(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M],
    KEY_EXTRACTOR_T && key_extractor = 0
)
{
    HW_STATIC_ASSERT(
        (
            POLICY_T == RR ||
            POLICY_T == LB ||
            POLICY_T == KB ||
            POLICY_T == BR
        ),
        "FX: Only RR, LB, KB and BR are supported policies!"
    );

#pragma HLS dataflow

Emitter:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        if constexpr (POLICY_T == RR) {
            fx::StoSN_RR<M>(istrms[i], ostrms[i], "Emitter_RR");
        } else if constexpr (POLICY_T == LB) {
            fx::StoSN_LB<M>(istrms[i], ostrms[i], "Emitter_LB");
        } else if constexpr (POLICY_T == KB) {
            fx::StoSN_KB<M>(istrms[i], ostrms[i], key_extractor, "Emitter_KB");
        } else if constexpr (POLICY_T == BR) {
            fx::StoSN_BR<M>(istrms[i], ostrms[i], "Emitter_BR");
        }
    }
}


//******************************************************************************
//
// Operator
//
//******************************************************************************

template <
    Operator_t OPERATOR_T,
    typename FUNCTOR_T,
    Policy_t IN_POLICY_T,
    Policy_t OUT_POLICY_T,
    int N,
    int M,
    int K,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T = int,
    typename KEY_GENERATOR_T = int
>
void ReplicateOperator(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[K],
    int i,
    KEY_EXTRACTOR_T && key_extractor = 0,
    KEY_GENERATOR_T && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            IN_POLICY_T == RR ||
            IN_POLICY_T == LB ||
            IN_POLICY_T == KB ||
            IN_POLICY_T == BR
        ),
        "FX: fx::A2A::ReplicateOperator IN_POLICY_T supports RR, LB, KB and BR policies only!"
    );

    HW_STATIC_ASSERT(
        (
            OPERATOR_T == MAP ||
            OPERATOR_T == FILTER ||
            OPERATOR_T == FLATMAP
        ),
        "FX: fx::A2A::ReplicateOperator OPERATOR_T supports MAP, FILTER and FLATMAP policies only!"
    );

    HW_STATIC_ASSERT(
        (
            OUT_POLICY_T == RR ||
            OUT_POLICY_T == LB ||
            OUT_POLICY_T == KB
        ),
        "FX: fx::A2A::ReplicateOperator OUT_POLICY_T supports RR, LB, and KB policies only!"
    );

    #pragma HLS dataflow

    // TODO: chose the right depth for the streams
    fx::stream<typename STREAM_IN::data_t, 16> snm_to;
    fx::stream<typename STREAM_OUT::data_t, 16> op_to_smk;

    if constexpr (IN_POLICY_T == RR) {
        fx::SNMtoS_RR<N, M>(istrms, snm_to, i, "ReplicateOperator_RR");
    } else if constexpr (IN_POLICY_T == LB) {
        fx::SNMtoS_LB<N, M>(istrms, snm_to, i, "ReplicateOperator_LB");
    } else if constexpr (IN_POLICY_T == KB) {
        fx::SNMtoS_KB<N, M>(istrms, snm_to, i, key_extractor, "ReplicateOperator_KB");
    }

    if constexpr (OPERATOR_T == MAP) {
        fx::Map<FUNCTOR_T>(snm_to, op_to_smk);
    } else if constexpr (OPERATOR_T == FILTER) {
        fx::Filter<FUNCTOR_T>(snm_to, op_to_smk);
    } else if constexpr (OPERATOR_T == FLATMAP) {
        fx::FlatMap<FUNCTOR_T>(snm_to, op_to_smk);
    }

    if constexpr (OUT_POLICY_T == RR) {
        fx::StoSN_RR<K>(op_to_smk, ostrms, "ReplicateOperator_RR");
    } else if constexpr (OUT_POLICY_T == LB) {
        fx::StoSN_LB<K>(op_to_smk, ostrms, "ReplicateOperator_LB");
    } else if constexpr (OUT_POLICY_T == KB) {
        fx::StoSN_KB<K>(op_to_smk, ostrms, key_generator, "ReplicateOperator_KB");
    } else if constexpr (OUT_POLICY_T == BR) {
        fx::StoSN_BR<K>(op_to_smk, ostrms, "ReplicateOperator_BR");
    }
}

template <
    Operator_t OPERATOR_T,
    typename FUNCTOR_T,
    Policy_t IN_POLICY_T,
    Policy_t OUT_POLICY_T,
    int N,
    int M,
    int K,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T = int,
    typename KEY_GENERATOR_T = int
>
void Operator(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][K],
    KEY_EXTRACTOR_T && key_extractor = 0,
    KEY_GENERATOR_T && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            IN_POLICY_T == RR ||
            IN_POLICY_T == LB ||
            IN_POLICY_T == KB ||
            IN_POLICY_T == BR
        ),
        "FX: fx::A2A::Operator IN_POLICY_T supports RR, LB, KB and BR policies only!"
    );

    HW_STATIC_ASSERT(
        (
            OPERATOR_T == MAP ||
            OPERATOR_T == FILTER ||
            OPERATOR_T == FLATMAP
        ),
        "FX: fx::A2A::Operator OPERATOR_T supports MAP, FILTER and FLATMAP policies only!"
    );

    HW_STATIC_ASSERT(
        (
            OUT_POLICY_T == RR ||
            OUT_POLICY_T == LB ||
            OUT_POLICY_T == KB
        ),
        "FX: fx::A2A::Operator OUT_POLICY_T supports RR, LB, KB and BR policies only!"
    );

    #pragma HLS dataflow
A2Aerator:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        ReplicateOperator<OPERATOR_T, FUNCTOR_T, IN_POLICY_T, OUT_POLICY_T, N, M, K>(
            istrms, ostrms[i], i, std::forward<KEY_EXTRACTOR_T>(key_extractor), std::forward<KEY_GENERATOR_T>(key_generator)
        );
    }
}


//******************************************************************************
//
// Generator
//
//******************************************************************************

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    int N,
    typename STREAM_OUT
>
void ReplicateGenerator(
    STREAM_OUT ostrms[N]
)
{
#pragma HLS dataflow
    fx::Generator<INDEX_T, FUNCTOR_T, STREAM_OUT> generator[N];
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        generator[i](ostrms[i]);
    }
}


//******************************************************************************
//
// Drainer
//
//******************************************************************************

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    int N,
    typename STREAM_IN
>
void ReplicateDrainer(
    STREAM_IN istrms[N]
)
{
#pragma HLS dataflow
    fx::Drainer<INDEX_T, FUNCTOR_T, STREAM_IN> drainer[N];
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        drainer[i](istrms[i]);
    }
}


//******************************************************************************
//
// Collector
//
//******************************************************************************

template <
    Policy_t POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_GENERATOR_T = int
>
void Collector(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M],
    KEY_GENERATOR_T && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            POLICY_T == RR ||
            POLICY_T == LB ||
            POLICY_T == KB
        ),
        "FX: fx::A2A::Collector supports RR, LB, and KB policies only!"
    );

#pragma HLS dataflow
Collector:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        if constexpr (POLICY_T == RR) {
            fx::SNMtoS_RR<N, M>(istrms, ostrms[i], i, "Collector_RR");
        } else if constexpr (POLICY_T == LB) {
            fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "Collector_LB");
        } else if constexpr (POLICY_T == KB) {
            fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, key_generator, "Collector_KB");
        }
    }
}

#else // USE_CONSTEXPR_IF_IMPLEMENTATION == 0

template <typename T>
constexpr auto default_key_extractor_t = [](const T & r){ return int(0); };
constexpr auto default_key_generator_t = [](const int index){ return int(0); };


template <
    Policy_t POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_EXTRACTOR_T = decltype(default_key_extractor_t<typename STREAM_IN::data_t>)
>
void Emitter(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M],
    KEY_EXTRACTOR_T & key_extractor = default_key_extractor_t<typename STREAM_IN::data_t>
)
{
    HW_STATIC_ASSERT(
        (
            POLICY_T == RR ||
            POLICY_T == LB ||
            POLICY_T == KB ||
            POLICY_T == BR
        ),
        "FX: fx::A2A::Emitter supports RR, LB, KB and BR policies only!"
    );

#pragma HLS dataflow

Emitter:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        if (POLICY_T == RR) {
            fx::StoSN_RR<M>(istrms[i], ostrms[i], "Emitter_RR");
        } else if (POLICY_T == LB) {
            fx::StoSN_LB<M>(istrms[i], ostrms[i], "Emitter_LB");
        } else if (POLICY_T == KB) {
            fx::StoSN_KB<M>(istrms[i], ostrms[i], key_extractor, "Emitter_KB");
        } else if (POLICY_T == BR) {
            fx::StoSN_B<M>(istrms[i], ostrms[i], "Emitter_BR");
        }
    }
}

template <
    Policy_t POLICY_T,
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename KEY_GENERATOR_T = decltype(default_key_generator_t)
>
void Collector(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M],
    KEY_GENERATOR_T & key_generator = default_key_generator_t
)
{
    HW_STATIC_ASSERT(
        (
            POLICY_T == RR ||
            POLICY_T == LB ||
            POLICY_T == KB
        ),
        "FX: fx::A2A::Collector supports RR, LB, and KB policies only!"
    );

#pragma HLS dataflow
Collector:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        if (POLICY_T == RR) {
            fx::SNMtoS_RR<N, M>(istrms, ostrms[i], i, "Collector_RR");
        } else if (POLICY_T == LB) {
            fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "Collector_LB");
        } else if (POLICY_T == KB) {
            fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, key_generator, "Collector_KB");
        }
    }
}

#endif // USE_CONSTEXPR_IF_IMPLEMENTATION == 0

} // namespace A2A
} // namespace fx

#endif // __CONNECTORS_ALL2ALL__
