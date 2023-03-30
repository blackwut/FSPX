#include "kernel.hpp"

bool test_s_to_s()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        int r = out.read();
        last = out.read_eos();

        if (i != r) {
            return false;
        }
        ++i;
    }

    return true;
}

int main(int argc, char * argv[])
{
    int err = 0;
    if (!test_s_to_s()) {
        err = -1;
    }
    return err;
}
