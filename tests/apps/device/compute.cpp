#include "fspx.hpp"

#include "constants.hpp"
#include "tuple.hpp"

#define SO_PAR 2
#define IN_PAR 2
#define SI_PAR 2

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    int N,
    typename STREAM_OUT
>
void replicate_generator(
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

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    int N,
    typename STREAM_IN
>
void replicate_drainer(
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

template <int N>
struct RecordGenerator {
    void operator()(const int i, record_t & out, bool & last) {
    #pragma HLS INLINE
        out.key = i;
        out.val = i;

        last = (i == N - 1);
    }
};

struct RecordDrainer {
    void operator()(const int i, const record_t & in, const bool last) {
    #pragma HLS INLINE
        // std::cout << "index:" << i << ", key: " << in.key << ", val: " << in.val << '\n';
    }
};



extern "C" {
void compute()
{
    #pragma HLS DATAFLOW

    stream_t in[SO_PAR];
    stream_t emitter_incrementer[SO_PAR][IN_PAR];
    stream_t incrementer_collector[IN_PAR][SI_PAR];
    stream_t out[SI_PAR];

    constexpr int N = 1 << 30;

    replicate_generator<int, RecordGenerator<N>, SO_PAR>(in);
    fx::A2A::Emitter_RR<SO_PAR, IN_PAR>(in, emitter_incrementer);
    A2A_Map<Incrementer, SO_PAR, IN_PAR, SI_PAR>(emitter_incrementer, incrementer_collector);
    fx::A2A::Collector_RR<IN_PAR, SI_PAR>(incrementer_collector, out);
    replicate_drainer<int, RecordDrainer, SI_PAR>(out);
}
}
