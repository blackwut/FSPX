#include "kernel.hpp"

#define SIZE 16

int main() {

    stream_t in;
    axis_stream_t out[PAR];

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out);

    for (int i = 0; i < SIZE / PAR; ++i) {
        for (int p = 0; p < PAR; ++p) {
            bool last = false;
            int r = out[p].read(last);
            if ((i * 4 + p) != r) {
                return 1;
            }

            if (last) {
                return 2;
            }
        }
    }

    for (int p = 0; p < PAR; ++p) {
        bool last = false;
        out[p].read(last);

        if (!last) {
            return 3;
        }
    }

    return 0;
}
