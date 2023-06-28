#include "kernel.hpp"

void test(
    stream_t & in,
    stream_t & out
)
{
    fx::StoS(in, out);
}
