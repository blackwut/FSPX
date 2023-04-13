#include "kernel.hpp"

#define SIZE 16

int main() {

    axis_stream_t in;
    stream_t out[PAR];

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out);

    for (int i = 0; i < SIZE / PAR; ++i) {
        for (int p = 0; p < PAR; ++p) {
            int r = out[p].read();
            bool last = out[p].read_eos();
            if ((i * 4 + p) != r) {
                return 1;
            }

            if (last) {
                return 2;
            }
        }
    }

    for (int p = 0; p < PAR; ++p) {
        bool last = out[p].read_eos();

        if (!last) {
            return 3;
        }
    }

    return 0;
}
