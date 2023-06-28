#include "kernel.hpp"

using stream_t = fx::stream<record_t, 8>;

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
void A2A_ReplicateMap(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][K]
)
{
// The following dataflow pragma will produce a warning but it is necessary to replicate the operator
#pragma HLS dataflow
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_map<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
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
void replicate_filter(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[K],
    int i
)
{
    #pragma HLS dataflow

    stream_t snm_to_fun;
    stream_t fun_to_smk;

    fx::SNMtoS_LB<N, M>(istrms, snm_to_fun, i, "filter");
    fx::Filter<FUNCTOR_T>(snm_to_fun, fun_to_smk);
    fx::StoSN_LB<K>(fun_to_smk, ostrms, "filter");
}

template <
    typename FUNCTOR_T,
    int N,
    int M,
    int K,
    typename STREAM_IN,
    typename STREAM_OUT
>
void A2A_ReplicateFilter(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][K]
)
{
// The following dataflow pragma will produce a warning but it is necessary to replicate the operator
#pragma HLS dataflow
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_filter<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
    }
}

template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void handle_inputs(
    STREAM_IN istrms[N],
    STREAM_OUT ostrms[N][M]
)
{
#pragma HLS dataflow

handle_inputs:
    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::StoSN_KB<M>(istrms[i], ostrms[i],
        [](const record_t & r) {
            return (int)(r.key % K);
        });
    }
}

template <
    int N,
    int M,
    typename STREAM_IN,
    typename STREAM_OUT
>
void handle_outputs(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[N]
)
{
#pragma HLS dataflow
handle_outputs:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "handle_outputs");
    }
}


void test(
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
)
{
    #pragma HLS dataflow

    stream_t source_predictor[SO_PAR][MA_PAR];
	stream_t predictor_filter[MA_PAR][SD_PAR];
    stream_t filter_sink[SD_PAR][SI_PAR];

// Source_LB:
//     for (int i = 0; i < SO_PAR; ++i) {
//     #pragma HLS unroll
//         fx::StoSN_LB<MA_PAR>(in[i], source_predictor[i], "source");
//     }

    handle_inputs<SO_PAR, MA_PAR>(in, source_predictor);

    A2A_ReplicateMap<MovingAverage<float>, SO_PAR, MA_PAR, SD_PAR>(source_predictor, predictor_filter);
    A2A_ReplicateFilter<SpikeDetector<float>, MA_PAR, SD_PAR, SI_PAR>(predictor_filter, filter_sink);

    handle_outputs<SD_PAR, SI_PAR>(filter_sink, out);

// Sink_LB:
//     for (int i = 0; i < SI_PAR; ++i) {
//     #pragma HLS unroll
//         fx::SNMtoS_LB<SD_PAR, SI_PAR>(filter_sink, out[i], i, "sink");
//     }
}
