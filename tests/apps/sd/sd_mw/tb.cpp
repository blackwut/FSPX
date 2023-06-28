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
        in.write(r, (i + 1) == in_count);
    }

    int n[K] = {0};
    int eos[K] = {false};

    int total_n = 0;

    for (int i = 0; i < K; ++i) {
		test(in, &out[i * (SIZE / K)], out_size_bytes / K, &n[i], &eos[i]);

		if (n[i] != SIZE / K) {
			DUMP_VAR(n[i]);
			return 1;
		}

		if (eos[i] != (i == K - 1)) {
			DUMP_VAR(eos[i]);
			return 2;
		}

		total_n += n[i];
    }

    for (int i = 0; i < total_n; ++i) {
		line_t l = new_line(i);

		// print_line(out[i]);
		//print_line(l);
		if (out[i] != l) {
			return 3;
		}
	}

    return 0;
}
