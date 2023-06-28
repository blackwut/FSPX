#include "kernel.hpp"

// FROM FSP SpikeDetector
// channel source_predictor_t source_predictor_ch[2][2];
// channel predictor_filter_t predictor_filter_ch[2][2];
// channel filter_sink_t filter_sink_ch[2];

#define STREAM_TRANSPOSE 0

#if STREAM_TRANSPOSE
namespace fx {
template <
    typename STREAM_IN,
    typename STREAM_OUT,
    int N,
    int M
>
void streams_transpose(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][N]
)
{
#pragma HLS dataflow

    for (int n = 0; n < N; ++n) {
    #pragma HLS unroll
        for (int m = 0; m < M; ++m) {
        #pragma HLS unroll
            fx::StoS(istrms[n][m], ostrms[m][n]);

            // bool last = istrms[n][m].read_eos();
            // while (!last) {
            // #pragma HLS PIPELINE II = 1
            //     ostrms[m][n].write(istrms[n][m].read());
            //     last = istrms[n][m].read_eos();
            // }
            // ostrms[m][n].write_eos();
        }
    }
}

template <
    int N,
    int M,
    int K,
    typename FUN_T,
    typename STREAM_IN,
    typename STREAM_OUT
>
void replicate_map(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[N][K],
    FUN_T f[N]
)
{
#pragma HLS dataflow

    stream_t sn_to_fun[N];
    stream_t fun_to_sn[N];

    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::SNtoS_LB<M>(istrms[i], sn_to_fun[i]);
        fx::Map(sn_to_fun[i], fun_to_sn[i], f[i]);
        fx::StoSN_LB<K>(fun_to_sn[i], ostrms[i]);
    }
}

template <
    int N,
    int M,
    int K,
    typename FUN_T,
    typename STREAM_IN,
    typename STREAM_OUT
>
void replicate_filter(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[N][K],
    FUN_T f[N]
)
{
#pragma HLS dataflow

    stream_t sn_to_fun[N];
    stream_t fun_to_sn[N];

    for (int i = 0; i < N; ++i) {
    #pragma HLS unroll
        fx::SNtoS_LB<M>(istrms[i], sn_to_fun[i]);
        fx::Filter(sn_to_fun[i], fun_to_sn[i], f[i]);
        fx::StoSN_LB<K>(fun_to_sn[i], ostrms[i]);
    }
}

}
#endif

struct map_t {
    void operator()(record_t x, record_t & y) {
        y = x;
    }
};

struct filter_t {
    void operator()(record_t x, record_t & y, bool & b) {
        y = x;
        b = true;
    }
};

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

    fx::SNMtoS_LB<N, M>(istrms, snm_to_fun, i);
    fx::Map<FUNCTOR_T>(snm_to_fun, fun_to_smk);
    fx::StoSN_LB<K>(fun_to_smk, ostrms);
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
// #pragma HLS dataflow

A2A_ReplicateMap:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_map<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
        // fx::SNMtoS_LB<N, M>(istrms, snm_to_fun[i], i);
        // fx::Map(snm_to_fun[i], fun_to_smk[i], func[i]);
        // fx::StoSN_LB<K>(fun_to_smk[i], ostrms[i]);
    }
}

// template <
//     int N,
//     int M,
//     int K,
//     typename STREAM_IN,
//     typename STREAM_OUT,
//     typename FUNCTOR_T
// >
// void A2A_ReplicateMap(
//     STREAM_IN istrms[N][M],
//     STREAM_OUT ostrms[M][K],
//     FUNCTOR_T func[M]
// )
// {
//     stream_t snm_to_fun[M];
//     stream_t fun_to_smk[M];

// #pragma HLS dataflow

//     for (int i = 0; i < M; ++i) {
//     #pragma HLS unroll
//         fx::SNMtoS_LB<N, M>(istrms, snm_to_fun[i], i, "A2A_ReplicateMap");
//         fx::Map(snm_to_fun[i], fun_to_smk[i], func[i]);
//         // fx::StoSNM_LB<M, K>(fun_to_smk[i], ostrms, i);
//         fx::StoSN_LB<K>(fun_to_smk[i], ostrms[i], "A2A_ReplicateMap");
//     }
// }

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

    fx::SNMtoS_LB<N, M>(istrms, snm_to_fun, i);
    fx::Filter<FUNCTOR_T>(snm_to_fun, fun_to_smk);
    fx::StoSN_LB<K>(fun_to_smk, ostrms);
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
// #pragma HLS dataflow

A2A_ReplicateFilter:
    for (int i = 0; i < M; ++i) {
    #pragma HLS unroll
        replicate_filter<FUNCTOR_T, N, M, K>(istrms, ostrms[i], i);
    }
}

// template <
//     int N,
//     int M,
//     int K,
//     typename STREAM_IN,
//     typename STREAM_OUT,
//     typename FUNCTOR_T
// >
// void A2A_ReplicateFilter(
//     STREAM_IN istrms[N][M],
//     STREAM_OUT ostrms[M][K],
//     FUNCTOR_T func[M]
// )
// {
//     stream_t snm_to_fun[M];
//     stream_t fun_to_smk[M];

// #pragma HLS dataflow

// A2A_ReplicateFilter:
//     for (int i = 0; i < M; ++i) {
//     #pragma HLS unroll
//         fx::SNMtoS_LB<N, M>(istrms, snm_to_fun[i], i, "A2A_ReplicateFilter");
//         fx::Filter(snm_to_fun[i], fun_to_smk[i], func[i]);
//         // fx::StoSNM_LB<M, K>(fun_to_smk[i], ostrms, i);
//         fx::StoSN_LB<K>(fun_to_smk[i], ostrms[i], "A2A_ReplicateFilter");
//     }
// }

void test(
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
)
{
    // #pragma HLS interface ap_ctrl_none port=return
	#pragma HLS INTERFACE mode=axis register_mode=both port=in[0]
    #pragma HLS INTERFACE mode=axis register_mode=both port=in[1]
    #pragma HLS INTERFACE mode=axis register_mode=both port=out[0]

#if !STREAM_TRANSPOSE
    stream_t source_predictor[SO_PAR][MA_PAR];
	stream_t predictor_filter[MA_PAR][SD_PAR];
	stream_t filter_sink[SD_PAR][SI_PAR];

	// map_t ma[MA_PAR];
	// filter_t sd[SD_PAR];
    

    #pragma HLS dataflow

    // static MovingAverage<float> ma[MA_PAR];
    // static SpikeDetector<float> sd[SD_PAR];

Source_LB:
    for (int i = 0; i < SO_PAR; ++i) {
    #pragma HLS unroll
        fx::StoSN_LB<MA_PAR>(in[i], source_predictor[i], "source");
    }

    // A2A_ReplicateMap<SO_PAR, MA_PAR, SD_PAR>(source_predictor, predictor_filter, ma);
    A2A_ReplicateMap<MovingAverage<float>, SO_PAR, MA_PAR, SD_PAR>(source_predictor, predictor_filter);
    A2A_ReplicateFilter<SpikeDetector<float>, MA_PAR, SD_PAR, SI_PAR>(predictor_filter, filter_sink);

Sink_LB:
    for (int i = 0; i < SI_PAR; ++i) {
    #pragma HLS unroll
        fx::SNMtoS_LB<SD_PAR, SI_PAR>(filter_sink, out[i], i, "sink");
    }
    

#else
    stream_t source_predictor[SO_PAR][MA_PAR];
	stream_t predictor_source[MA_PAR][SO_PAR];

	stream_t predictor_filter[MA_PAR][SD_PAR];
	stream_t filter_predictor[SD_PAR][MA_PAR];

	stream_t filter_sink[SD_PAR][SI_PAR];
	stream_t sink_filter[SI_PAR][SD_PAR];

	map_t ma[MA_PAR]{};
	filter_t sd[SD_PAR]{};

    #pragma HLS dataflow

    // (in0 axis_stream) 1 to N
    // (in1 axis_stream) 1 to N


    for (int s = 0; s < SO_PAR; ++s) {
        fx::StoSN_LB<MA_PAR>(in[s], source_predictor[s]);
    }

    // N replicas of MovingAverage composed of:
    // (in0 and in1) 2 to 1
    // MA replica
    // 1 to M (connecting to SpikeDetector replicas, which are M)

    fx::streams_transpose(source_predictor, predictor_source);
    fx::replicate_map<MA_PAR>(predictor_source, predictor_filter, ma);

    // M replicas of SpikeDetector composed of:
    // N to 1
    // SD replica
    // output to Collector
    // SpikeDetector sd[SD_PAR]{};

    fx::streams_transpose(predictor_filter, filter_predictor);
    fx::replicate_filter<SD_PAR>(filter_predictor, filter_sink, sd);

    // Collector: M to 1 (out axis_stream)
    fx::streams_transpose(filter_sink, sink_filter);
    for (int s = 0; s < SI_PAR; ++s) {
        fx::SNtoS_LB<SD_PAR>(sink_filter[s], out[s]);
    }
#endif
}
