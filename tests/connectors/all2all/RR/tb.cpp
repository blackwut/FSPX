#include "kernel.hpp"

data_t new_data(int i)
{
    data_t d;
    d.key = i;
    d.val = i;
    return d;
}

bool check_data(const data_t & r, int i)
{
    return (r == new_data(i));
}

void print_data(const data_t & r)
{
    std::cout << "(" << r.key << ", " << r.val << ")" << std::endl;
}

int main(int argc, char * argv[])
{
    stream_t in[IN_PAR];
    stream_t out[OUT_PAR];

    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < IN_PAR; ++j) {
            data_t d = new_data(i * IN_PAR + j);
            in[j].write(d);
            // print_data(d);
        }
    }

    for (int j = 0; j < IN_PAR; ++j) {
        in[j].write_eos();
    }

    test(in, out);

    for (int i = 0; i < OUT_PAR; ++i) {
        int j = 0;
        bool last = out[i].read_eos();
        while (!last) {
            data_t r = out[i].read();
            last = out[i].read_eos();

            // std::cout << i << " " << j << " ";
            // print_data(r);

            int index = (j % IN_PAR) + ((j - (j % IN_PAR)) * OUT_PAR) + (i * IN_PAR);

            if (!check_data(r, index)) {
                print_data(r);
                return 1;
            }
            ++j;
        }
    }

    return 0;
}
