#include "kernel.hpp"

int main() {

    line_t in[SIZE];
    axis_stream_t out("out");

    for (int i = 0; i < SIZE; ++i) {
        line_t l = new_line(i);
        // print_line(l);
        in[i] = new_line(i);
    }

    test(in, out, SIZE);

    int i = 0;
    int j = 0;
    bool last = false;
    record_t r = out.read(last);
    while (!last) {
        line_t line = in[i];
        ap_uint<ITEM_BITS> item = line.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j);
        record_t in_r = TypeHandler<record_t>::from_ap(item);
        // print_record(r);

        if (r != in_r) {
            return 1;
        }

        ++j;
        if (j == READ_WRITE_ITEMS) {
            j = 0;
            ++i;
        }

        r = out.read(last);
    }

    return 0;
}
