#include "kernel.hpp"

int main() {

    axis_stream_t in("in");
    axis_stream_t out("out");

    for (int i = 0; i < SIZE; ++i) {
        record_t r = new_record(i);
        in.write(r);
    }
    in.write_eos();

    test(in, out);

    MovingAverage<float> ma{};
    SpikeDetector sd{};

    int i = 0;
    bool last = false;
    record_t r = out.read(last);
    while (!last) {
        record_t r_test = new_record(i);
        record_t r_ma = ma(r_test);
        record_t r_sd;
        bool res = sd(r_ma, r_sd);
        if (res) {
            // print_record(r_sd);
            // print_record(r);
            if (r != r_sd) {
                return 1;
            }
            r = out.read(last);
        }
        ++i;
    }

    return 0;
}