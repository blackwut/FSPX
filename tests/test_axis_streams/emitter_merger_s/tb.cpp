#include "kernel.hpp"

#define SIZE 16

int main() {

    stream_t in;
    axis_stream_t internal_streams[PAR];
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    // test_emitter
    test(in, internal_streams);
    fx::ANtoS_RR<PAR>(internal_streams, out);

    // test_merger
    // fx::StoAN_RR<PAR>(in, internal_streams);
    // test(internal_streams, out);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        data_t r = out.read();
        last = out.read_eos();

        if (i != r) {
            return 1;
        }
        i++;
    }

    return 0;
}
