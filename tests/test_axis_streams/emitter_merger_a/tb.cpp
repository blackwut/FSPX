#include "kernel.hpp"

#define SIZE 16

int main() {

    axis_stream_t in;
    axis_stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out);

    int i = 0;
    bool last = false;
    data_t r = out.read(last);
    while (!last) {
        if (i != r) {
            return 1;
        }
        r = out.read(last);
        i++;
    }

    // r = out.read(last);
    // if (!last) {
    //     return 2;
    // }

    return 0;
}
