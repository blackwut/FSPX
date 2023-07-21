#include "fspx.hpp"

#include "constants.hpp"
#include "tuple.hpp"

// #define MR_PAR 4
// #define MAP_PAR 4
// #define MW_PAR 4


template <
    typename FUNCTOR_T,
    int N,
    int M,
    int K,
    typename STREAM_IN,
    typename STREAM_OUT
>
void replicate_map(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[K],
    int i
)
{
    #pragma HLS dataflow

    stream_t snm_to_fun;
    stream_t fun_to_smk;

    fx::SNMtoS_LB<N, M>(istrms, snm_to_fun, i, "map");
    fx::Map<FUNCTOR_T>(snm_to_fun, fun_to_smk);
    fx::StoSN_LB<K>(fun_to_smk, ostrms, "map");
}

template <
    typename FUNCTOR_T,
    int N,
    int M,
    int K,
    typename STREAM_IN,
    typename STREAM_OUT
>
void A2A_Map(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][K]
)
{
// The following dataflow pragma will produce a warning but it is necessary to replicate the operator
#warning "FX: The following warning is expected!"
#pragma HLS dataflow

A2A_Map:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_map<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
    }
}

struct Incrementer {
    void operator()(const record_t & in, record_t & out) {
    #pragma HLS INLINE
        out.key = in.key;
        out.val = in.val + 1;
    }
};


extern "C" {
void compute(
    axis_stream_t in[MR_PAR],
    axis_stream_t out[MW_PAR]
)
{
    #pragma HLS interface ap_ctrl_none port=return

    #pragma HLS DATAFLOW

    stream_t emitter_incrementer[MR_PAR][MAP_PAR];
    stream_t incrementer_collector[MAP_PAR][MW_PAR];

    fx::A2A::Emitter_RR<MR_PAR, MAP_PAR>(in, emitter_incrementer);
    A2A_Map<Incrementer, MR_PAR, MAP_PAR, MW_PAR>(emitter_incrementer, incrementer_collector);
    fx::A2A::Collector_RR<MAP_PAR, MW_PAR>(incrementer_collector, out);
}
}
