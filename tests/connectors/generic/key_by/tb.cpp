#include "kernel.hpp"

data_t new_data_in(int i)
{
    data_t d;
    d.x[0] = i >> 24;
    d.x[1] = i >> 16;
    d.x[2] = i >> 8;
    d.x[3] = i;
    d.y[0] = i >> 16;
    d.y[1] = i;
    d.z = i;
    return d;
}

data_t new_data_out(int i)
{
    i = (i / 4) * 4 + (PAR - (i % PAR) - 1);
    return new_data_in(i);
}

bool check_data(const data_t & r, int i)
{
    data_t d = new_data_out(i);
    return (d == r);
}

bool test_round_robin()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(new_data_in(i));
    }
    in.write_eos();

    test(in, out);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        data_t r = out.read();
        last = out.read_eos();

        if (!check_data(r, i)) {
            return 1;
        }
        ++i;
    }

    return 0;
}

int main(int argc, char * argv[])
{
    return test_round_robin();
}
