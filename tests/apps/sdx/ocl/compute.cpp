#include "fspx.hpp"

#include "tuple.hpp"
#include "constants.hpp"
#include "moving_average.hpp"
#include "spike_detector.hpp"

#define SO_PAR 2
#define MA_PAR 2
#define SD_PAR 2
#define SI_PAR 1

using stream_t = fx::stream<record_t, 16>;

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
void A2A_Filter(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][K]
)
{
// The following dataflow pragma will produce a warning but it is necessary to replicate the operator
#warning "FX: The following warning is expected!"
#pragma HLS dataflow
A2A_Filter:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_filter<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
    }
}

// template <
//     int N,
//     int M,
//     typename STREAM_IN,
//     typename STREAM_OUT
// >
// void A2A_Source(
//     STREAM_IN istrms[N],
//     STREAM_OUT ostrms[N][M]
// )
// {
// #warning "FX: The following warning is expected!"
// #pragma HLS dataflow

// A2A_Source:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         fx::StoSN_KB<M>(istrms[i], ostrms[i],
//         [](const record_t & r) {
//             return (int)(r.key % K);
//         },
//         "A2A_Source");
//     }
// }

// template <
//     int N,
//     int M,
//     typename STREAM_IN,
//     typename STREAM_OUT
// >
// void A2A_Sink(
//     STREAM_IN istrms[N][M],
//     STREAM_OUT ostrms[M]
// )
// {
// #warning "FX: The following warning is expected!"
// #pragma HLS dataflow
// A2A_Sink:
//     for (int i = 0; i < M; ++i) {
//     #pragma HLS unroll
//         fx::SNMtoS_LB<N, M>(istrms, ostrms[i], i, "A2A_Sink");
//     }
// }


extern "C" {
void compute(
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
)
{
    #pragma HLS interface ap_ctrl_none port=return

    #pragma HLS DATAFLOW

    stream_t source_predictor[SO_PAR][MA_PAR];
    stream_t predictor_filter[MA_PAR][SD_PAR];
    stream_t filter_sink[SD_PAR][SI_PAR];

    // A2A_Source<SO_PAR, MA_PAR>(in, source_predictor);
    fx::A2A::Emitter_KB<SO_PAR, MA_PAR>(in, source_predictor,
        [](const record_t & r) {
            return (int)(r.key % MA_PAR);
        }
    );
    A2A_Map<MovingAverage<float>, SO_PAR, MA_PAR, SD_PAR>(source_predictor, predictor_filter);
    A2A_Filter<SpikeDetector<float>, MA_PAR, SD_PAR, SI_PAR>(predictor_filter, filter_sink);
    // A2A_Sink<SD_PAR, SI_PAR>(filter_sink, out);
    fx::A2A::Collector_LB<SD_PAR, SI_PAR>(filter_sink, out);
}
}
