#include "kernel.hpp"

bool test_broadcast()
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
        for (int p = 0; p < PAR; ++p) {
            int r = out.read();
            last = out.read_eos();

            if (i != r) {
                return false;
            }
        }
        ++i;
    }

    return true;
}

int main(int argc, char * argv[])
{
    int err = 0;
    if (!test_broadcast()) {
        err = -1;
    }
    return err;
}
