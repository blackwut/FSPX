#include "kernel.hpp"

int main() {

    axis_stream_t in("in");
    line_t out[SIZE];

    int in_count = READ_WRITE_ITEMS * SIZE;
    int out_size_bytes = sizeof(out);

    DUMP_VAR(SIZE);
    DUMP_VAR(in_count);

    for (int i = 0; i < in_count; ++i) {
        record_t r = new_record(i);
        // print_record(r);
        in.write(r);
    }
    in.write_eos();

    int n = 0;
    int eos = 0;
    test(in, out, out_size_bytes, &n, &eos);

    if (n != SIZE) {
        DUMP_VAR(n);
        return 1;
    }

    if (eos != 1) {
        DUMP_VAR(eos);
        return 2;
    }

    for (int i = 0; i < n; ++i) {
        line_t l = new_line(i);

        // print_line(out[i]);
        if (out[i] != l) {
            return 3;
        }
    }

    return 0;
}
