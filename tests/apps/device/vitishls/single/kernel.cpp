#include "kernel.hpp"

using stream_t = fx::stream<record_t, 8>;

// template <
//     typename INDEX_T,
//     typename FUNCTOR_T,
//     int N,
//     typename STREAM_OUT
// >
// void replicate_generator(
//     STREAM_OUT ostrms[N]
// )
// {
// #pragma HLS dataflow
//     fx::Generator<INDEX_T, FUNCTOR_T, STREAM_OUT> generator[N];
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         generator[i](ostrms[i]);
//     }
// }

// template <
//     typename INDEX_T,
//     typename FUNCTOR_T,
//     int N,
//     typename STREAM_IN
// >
// void replicate_drainer(
//     STREAM_IN istrms[N]
// )
// {
// #pragma HLS dataflow
//     fx::Drainer<INDEX_T, FUNCTOR_T, STREAM_IN> drainer[N];
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         drainer[i](istrms[i]);
//     }
// }

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



void compute()
{
    #pragma HLS DATAFLOW

    stream_t in[SO_PAR];
    stream_t emitter_incrementer[SO_PAR][IN_PAR];
    stream_t incrementer_collector[IN_PAR][SI_PAR];
    stream_t out[SI_PAR];

    constexpr int N = 16;

    // replicate_generator<int, RecordGenerator<N>, SO_PAR>(in);
    fx::A2A::ReplicateGenerator<int, RecordGenerator<N>, SO_PAR>(in);

    // fx::A2A::Emitter_RR<SO_PAR, IN_PAR>(in, emitter_incrementer);
    fx::A2A::Emitter<fx::A2A::Policy_t::RR, SO_PAR, IN_PAR>(in, emitter_incrementer);

    // A2A_Map<Incrementer, SO_PAR, IN_PAR, SI_PAR>(emitter_incrementer, incrementer_collector);
    // fx::A2A::Operator<fx::Map<Incrementer>, fx::A2A::Policy_t::LB, fx::A2A::Policy_t::LB, SO_PAR, IN_PAR, SI_PAR>(emitter_incrementer, incrementer_collector);

    fx::A2A::Operator<fx::A2A::Operator_t::MAP, Incrementer, fx::A2A::Policy_t::LB, fx::A2A::Policy_t::LB, SO_PAR, IN_PAR, SI_PAR>(emitter_incrementer, incrementer_collector);

    // fx::A2A::Map<average_calculator, fx::A2A::LB, fx::A2A::RR, SOURCE_PAR, AVERAGE_CALCULATOR_PAR, SPIKE_DETECTOR_PAR>(
    //     source_average_calculator, average_calculator_spike_detector
    // );

    // fx::A2A::Collector_RR<IN_PAR, SI_PAR>(incrementer_collector, out);
    fx::A2A::Collector<fx::A2A::Policy_t::RR, IN_PAR, SI_PAR>(incrementer_collector, out);

    // replicate_drainer<int, RecordDrainer, SI_PAR>(out);
    fx::A2A::ReplicateDrainer<int, RecordDrainer, SI_PAR>(out);
}
