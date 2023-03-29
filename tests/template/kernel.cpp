#include "kernel.hpp"

void test(
    fx::stream<int> & in,
    fx::stream<int> & out,
	int size
)
{
	for (int i = 0; i < size; ++i) {
	#pragma HLS PIPELINE II=1
		int d = in.read();
		out.write(d);
	}
	out.write_eos();
}
