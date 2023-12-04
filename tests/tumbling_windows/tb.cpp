#include "kernel.hpp"

int main() {
    in_stream_t in("in");
    out_stream_t out("out");

    for (int i = 0; i < DATA_SIZE; ++i) {
        data_t d;
        d.key = i % MAX_KEYS;
        d.value = i;
        in.write(d);
    }
    in.write_eos();

    test(in, out);

    bool last = out.read_eos();
    while (!last) {
        data_t r = out.read();
        last = out.read_eos();
        std::cout << r.key << ", " << r.value << ", " << r.aggregate << std::endl;
    }

    return 0;
}
