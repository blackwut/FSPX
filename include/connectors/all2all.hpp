#ifndef __CONNECTORS_ALL2ALL__
#define __CONNECTORS_ALL2ALL__

#include <utility>
#include <type_traits>
#include "../common.hpp"
#include "../streams/streams.hpp"
#include "../operators/operators.hpp"
#include "generic.hpp"

// #pragma GCC system_header

namespace fx {
namespace A2A {

enum policy_t {
    RR,
    LB,
    KB,
    BR
};

enum operator_t {
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

template <
    policy_t policy_out,
    unsigned int N,
    unsigned int M,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t = int
>
void Emitter (
    stream_in_t istrms[N],
    stream_out_t ostrms[N][M],
    key_extractor_t && key_extractor = 0
)
{
    HW_STATIC_ASSERT(
        (
            policy_out == RR ||
            policy_out == LB ||
            policy_out == KB ||
            policy_out == BR
        ),
        "FX: Only RR, LB, KB and BR are supported policies!"
    );

    #pragma HLS DATAFLOW

    Emitter:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        if (policy_out == RR) {
            fx::StoSN_RR<M>(istrms[i], ostrms[i], "Emitter_RR");
        } else if (policy_out == LB) {
            fx::StoSN_LB<M>(istrms[i], ostrms[i], "Emitter_LB");
        } else if (policy_out == KB) {
            fx::StoSN_KB<M>(istrms[i], ostrms[i], std::forward<key_extractor_t>(key_extractor), "Emitter_KB");
        } else if (policy_out == BR) {
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
    operator_t op,
    typename functor_t,
    policy_t policy_in,
    policy_t policy_out,
    unsigned int N,
    unsigned int M,
    unsigned int K,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t = int,
    typename key_generator_t = int
>
void ReplicateOperator (
    stream_in_t istrms[N][M],
    stream_out_t ostrms[K],
    unsigned int m,
    key_extractor_t && key_extractor = 0,
    key_generator_t && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            policy_in == RR ||
            policy_in == LB ||
            policy_in == KB ||
            policy_in == BR
        ),
        "FX: fx::A2A::ReplicateOperator policy_in supports RR, LB, KB and BR policies only!"
    );

    HW_STATIC_ASSERT(
        (
            op == MAP ||
            op == FILTER ||
            op == FLATMAP
        ),
        "FX: fx::A2A::ReplicateOperator op supports MAP, FILTER and FLATMAP operators only!"
    );

    HW_STATIC_ASSERT(
        (
            policy_out == RR ||
            policy_out == LB ||
            policy_out == KB
        ),
        "FX: fx::A2A::ReplicateOperator policy_out supports RR, LB, and KB policies only!"
    );

    #pragma HLS DATAFLOW

    // TODO: chose the right depth for the streams
    fx::stream<typename stream_in_t::data_t, 16> snm_to_op;
    fx::stream<typename stream_out_t::data_t, 16> op_to_smk;

    if (policy_in == RR) {
        fx::SNMtoS_RR<N, M>(istrms, snm_to_op, m, "ReplicateOperator_IN_POLICY_RR");
    } else if (policy_in == LB) {
        fx::SNMtoS_LB<N, M>(istrms, snm_to_op, m, "ReplicateOperator_IN_POLICY_LB");
    } else if (policy_in == KB) {
        fx::SNMtoS_KB<N, M>(istrms, snm_to_op, m, std::forward<key_generator_t>(key_generator), "ReplicateOperator_IN_POLICY_KB");
    }

    if (op == MAP) {
        fx::Map<functor_t>(snm_to_op, op_to_smk);
    } else if (op == FILTER) {
        fx::Filter<functor_t>(snm_to_op, op_to_smk);
    } else if (op == FLATMAP) {
        fx::FlatMap<functor_t>(snm_to_op, op_to_smk);
    }

    if (policy_out == RR) {
        fx::StoSN_RR<K>(op_to_smk, ostrms, "ReplicateOperator_OUT_POLICY_RR");
    } else if (policy_out == LB) {
        fx::StoSN_LB<K>(op_to_smk, ostrms, "ReplicateOperator_OUT_POLICY_LB");
    } else if (policy_out == KB) {
        fx::StoSN_KB<K>(op_to_smk, ostrms, std::forward<key_extractor_t>(key_extractor), "ReplicateOperator_OUT_POLICY_KB");
    } else if (policy_out == BR) {
        fx::StoSN_BR<K>(op_to_smk, ostrms, "ReplicateOperator_OUT_POLICY_BR");
    }
}

template <
    operator_t op,
    typename functor_t,
    policy_t policy_in,
    policy_t policy_out,
    unsigned int N,
    unsigned int M,
    unsigned int K,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t = int,
    typename key_generator_t = int
>
void Operator (
    stream_in_t istrms[N][M],
    stream_out_t ostrms[M][K],
    key_extractor_t && key_extractor = 0,
    key_generator_t && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            policy_in == RR ||
            policy_in == LB ||
            policy_in == KB ||
            policy_in == BR
        ),
        "FX: fx::A2A::Operator policy_in supports RR, LB, KB and BR policies only!"
    );

    HW_STATIC_ASSERT(
        (
            op == MAP ||
            op == FILTER ||
            op == FLATMAP
        ),
        "FX: fx::A2A::Operator op supports MAP, FILTER and FLATMAP operators only!"
    );

    HW_STATIC_ASSERT(
        (
            policy_out == RR ||
            policy_out == LB ||
            policy_out == KB
        ),
        "FX: fx::A2A::Operator policy_out supports RR, LB, KB and BR policies only!"
    );

    #pragma HLS DATAFLOW
    
    A2AOperator:
    for (int i = 0; i < M; ++i) {
        #pragma HLS UNROLL
        
        ReplicateOperator<op, functor_t, policy_in, policy_out, N, M, K>(
            istrms, ostrms[i], i, std::forward<key_extractor_t>(key_extractor), std::forward<key_generator_t>(key_generator)
        );
    }
}


//******************************************************************************
//
// Collector
//
//******************************************************************************

template <
    policy_t policy_in,
    unsigned int N,
    unsigned int M,
    typename stream_in_t,
    typename stream_out_t,
    typename key_generator_t = int
>
void Collector (
    stream_in_t istrms[N][M],
    stream_out_t ostrms[M],
    key_generator_t && key_generator = 0
)
{
    HW_STATIC_ASSERT(
        (
            policy_in == RR ||
            policy_in == LB ||
            policy_in == KB
        ),
        "FX: fx::A2A::Collector supports RR, LB, and KB policies only!"
    );

    #pragma HLS DATAFLOW
    
    Collector:
    for (int i = 0; i < M; ++i) {
        #pragma HLS UNROLL
        
        if (policy_in == RR) {
            fx::SNMtoS_RR<N, M>(istrms, ostrms[i], i, "Collector_RR");
        } else if (policy_in == LB) {
            fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "Collector_LB");
        } else if (policy_in == KB) {
            fx::SNMtoS_KB<N, M>(istrms, ostrms[i], i, std::forward<key_generator_t>(key_generator), "Collector_KB");
        }
    }
}


//******************************************************************************
//
// Generator
//
//******************************************************************************

template <
    typename index_t,
    typename functor_t,
    unsigned int N,
    typename stream_out_t,
    typename... Args
>
void ReplicateGenerator (
    stream_out_t ostrms[N],
    Args&&... args
)
{
    #pragma HLS DATAFLOW
    
    ReplicateGenerator:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        fx::Generator<index_t, functor_t, stream_out_t>(ostrms[i], std::forward<Args>(args)...);
    }
}


//******************************************************************************
//
// Drainer
//
//******************************************************************************

template <
    typename index_t,
    typename functor_t,
    unsigned int N,
    typename stream_in_t,
    typename... Args
>
void ReplicateDrainer (
    stream_in_t istrms[N],
    Args&&... args
)
{
    #pragma HLS DATAFLOW
    
    ReplicateDrainer:
    for (unsigned int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
        
        fx::Drainer<index_t, functor_t, stream_in_t>(istrms[i], std::forward<Args>(args)...);
    }
}

} // namespace A2A
} // namespace fx

#endif // __CONNECTORS_ALL2ALL__
