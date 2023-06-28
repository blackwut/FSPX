#include "kernel.hpp"

int main() {

    stream_t stream;

    axis_stream_t in[SO_PAR];
    axis_stream_t out[SI_PAR];

    for (int i = 0; i < SIZE; ++i) {
        record_t r = new_record_spike(i);
        for (int j = 0; j < SO_PAR; ++j){
            in[j].write(r);
        }   
    }

    for (int i = 0; i < SO_PAR; ++i) {
        in[i].write_eos();
    }

    test(in, out);

    // MovingAverage<float> ma{};
    // SpikeDetector sd{};

    // int i = 0;
    // bool last = false;
    // record_t r = out.read(last);
    // while (!last) {
    //     record_t r_test = new_record(i);
    //     record_t r_ma = ma(r_test);
    //     record_t r_sd;
    //     bool res = sd(r_ma, r_sd);
    //     if (res) {
    //         // print_record(r_sd);
    //         // print_record(r);
    //         if (r != r_sd) {
    //             return 1;
    //         }
    //         r = out.read(last);
    //     }
    //     ++i;
    // }

    for (int i = 0; i < SI_PAR; ++i) {
        bool last = out[i].read_eos();
        while (!last) {
            record_t r = out[i].read();
            last = out[i].read_eos();
            print_record(r);
        }
    }

    return 0;
}
