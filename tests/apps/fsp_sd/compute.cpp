#include "fspx.hpp"

#include "tuple.hpp"
#include "constants.hpp"
#include "moving_average.hpp"
#include "spike_detector.hpp"

using stream_t = fx::stream<record_t, 8>;


// FROM FSP SpikeDetector
// channel source_predictor_t source_predictor_ch[2][2];
// channel predictor_filter_t predictor_filter_ch[2][2];
// channel filter_sink_t filter_sink_ch[2];




namespace fx {
template <
    STREAM_IN,
    STREAM_OUT,
    int N,
    int M
>
void streams_transpose(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[M][N]
)
{
    for (int n = 0; n < N; ++n) {
    #pragma HLS UNROLL
        for (int m = 0; m < M; ++m) {
        #pragma HLS UNROLL
            ostrms[m][n].write(istrms[n][m].read());
        }
    }
}

template <
    FUN_T,
    STREAM_IN,
    STREAM_OUT,
    int N,
    int M,
    int K
>
void replicate(
    STREAM_IN istrms[N][M],
    STREAM_OUT ostrms[N][K],
    FUN_T f[N]
)
{
    stream_t sn_to_fun[N];
    stream_t fun_to_sn[N];

    for (int i = 0; i < N; ++i) {
        fx::SNtoS_LB(istrms[i], sn_to_fun[i]);
        fx::Map(sn_to_fun[i], fun_to_sn[i], f[i]);
        fx::SNtoS_LB(fun_to_sn[i], ostrms[i]);
    }
}

}

#define SO_PAR 2
#define MA_PAR 2
#define SD_PAR 2
#define SI_PAR 1

extern "C" {
void compute(
    axis_stream_t in[SO_PAR],
    axis_stream_t out[SI_PAR]
)
{
    #pragma HLS interface ap_ctrl_none port=return


    #pragma HLS DATAFLOW

    // (in0 axis_stream) 1 to N
    // (in1 axis_stream) 1 to N

    stream_t source_predictor[SO_PAR][MA_PAR];
    stream_t predictor_source[MA_PAR][SO_PAR];

    stream_t predictor_filter[MA_PAR][SD_PAR];
    stream_t filter_predictor[SD_PAR][MA_PAR];

    stream_t filter_sink[SD_PAR][SI_PAR];
    stream_t sink_filter[SI_PAR][SD_PAR];

    for (int s = 0; s < SO_PAR; ++s) {
        fx::StoSN_LB(in[s], source_predictor[s]);
    }

    // N replicas of MovingAverage composed of:
    // (in0 and in1) 2 to 1
    // MA replica
    // 1 to M (connecting to SpikeDetector replicas, which are M)
    static MovingAverage<float> ma[MA_PAR]{};
    fx::streams_transpose(source_predictor, predictor_source);
    fx::replicate(predictor_source, predictor_filter, ma);

    // M replicas of SpikeDetector composed of:
    // N to 1
    // SD replica
    // output to Collector
    static SpikeDetector sd[SD_PAR]{};
    fx::streams_transpose(predictor_filter, filter_predictor);
    fx::replicate(filter_predictor, filter_sink, sd);

    // Collector: M to 1 (out axis_stream)
    fx::streams_transpose(filter_sink, sink_filter);
    for (int s = 0; s < SI_PAR; ++s) {
        fx::SNtoS_LB(sink_filter[s], out[s]);
    }

// original
    // static MovingAverage<float> ma{};
    // stream_t internal_stream;
    // static SpikeDetector sd{};

    // fx::Map(in, internal_stream, ma);
    // fx::Filter(internal_stream, out, sd);
}
}
