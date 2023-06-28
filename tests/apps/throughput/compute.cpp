#include "fspx.hpp"

#include "constants.hpp"
#include "tuple.hpp"

#define SO_PAR 2
#define IN_PAR 2
#define SI_PAR 2


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
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
)
{
    #pragma HLS interface ap_ctrl_none port=return

    #pragma HLS DATAFLOW

    stream_t emitter_incrementer[SO_PAR][IN_PAR];
    stream_t incrementer_collector[IN_PAR][SI_PAR];

    fx::A2A::Emitter_RR<SO_PAR, IN_PAR>(in, emitter_incrementer);
    A2A_Map<Incrementer, SO_PAR, IN_PAR, SI_PAR>(emitter_incrementer, incrementer_collector);
    fx::A2A::Collector_RR<IN_PAR, SI_PAR>(incrementer_collector, out);
}
}
