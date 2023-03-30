#include "kernel.hpp"

void test(
    fx::stream<int> & in,
    fx::stream<int> & out,
)
{
    bool last = false;
    while (!last) {
    #pragma HLS PIPELINE II=1
        int d = in.read();
        last = in.read_eos();
        out.write(d);
    }
    out.write_eos();
}
