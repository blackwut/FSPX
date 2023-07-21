#include "fspx.hpp"
#include "defines.hpp"
#include "tuples/record_t.hpp"
#include "operators/moving_average.hpp"
#include "operators/spike_detector.hpp"

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

extern "C" {
void compute(
    axis_stream_t in[MR_PAR],
    axis_stream_t out[MW_PAR]
)
{
    #pragma HLS interface ap_ctrl_none port=return

    #pragma HLS DATAFLOW

    stream_t mr_ma[MR_PAR][MA_PAR];
    stream_t ma_sd[MA_PAR][SD_PAR];
    stream_t sd_mw[SD_PAR][MW_PAR];

    fx::A2A::Emitter_KB<MR_PAR, MA_PAR>(
        in, mr_ma,
        [](const record_t & r) {
            return (int)(r.key % MA_PAR);
        }
    );
    A2A_Map<MovingAverage<float>, MR_PAR, MA_PAR, SD_PAR>(mr_ma, ma_sd);
    A2A_Filter<SpikeDetector<float>, MA_PAR, SD_PAR, MW_PAR>(ma_sd, sd_mw);
    fx::A2A::Collector_LB<SD_PAR, MW_PAR>(sd_mw, out);
}
}
