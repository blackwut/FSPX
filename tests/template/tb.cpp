#include "kernel.hpp"

#define SIZE 16

int main() {

	int err = 0;

	fx::stream<int> in;
	fx::stream<int> out;

	for (int i = 0; i < SIZE; ++i) {
		in.write(i);
	}

	test(in, out, SIZE);

	for (int i = 0; i < SIZE; ++i) {
		int r = out.read();
		if (i != r) {
			err = -1;
		}
	}

	out.read_eos();

	return err;
}