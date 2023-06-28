#include "kernel.hpp"

int main() {

    axis_stream_t in("in");
    line_t out[K + 1][SIZE];

    int in_count = READ_WRITE_ITEMS * SIZE * K;
    int out_size_bytes = sizeof(out[0]);

    // DUMP_VAR(K);
    // DUMP_VAR(READ_WRITE_ITEMS);
    // DUMP_VAR(SIZE);
    // DUMP_VAR(in_count);

    // std::cout << "writing to in" << std::endl;
    for (int i = 0; i < in_count; ++i) {
        record_t r = new_record(i);
        // print_record(r);
        in.write(r);
    }
    in.write_eos();

    int n[K + 1];
    int eos[K + 1];

    for (int i = 0; i < K + 1; ++i) {
        n[i] = 0;
        eos[i] = 0;
    }

    for (int i = 0; i < K + 1; ++i) {
        // std::cout << "launching kernel i = " << i << std::endl;
        test(in, out[i], out_size_bytes, &n[i], &eos[i]);

        // DUMP_VAR(n[i]);
        // DUMP_VAR(eos[i]);

        if (n[i] != SIZE * (i < K)) {
            return 1;
        }

        if (eos[i] != (i == K)) {
            return 2;
        }
    }

    for (int i = 0; i < K; ++i) {
        // std::cout << "checking kernel i = " << i << std::endl;
        for (int j = 0; j < SIZE; ++j) {
            line_t l = new_line(i * SIZE + j);
            // print_line(l);
            // print_line(out[i][j]);

            if (out[i][j] != l) {
                return 3;
            }
        }
    }

    return 0;
}
