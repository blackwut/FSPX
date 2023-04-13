#include "kernel.hpp"

void test(
    axis_stream_t & in,
    stream_t out[PAR]
)
{
    fx::AtoSN_LB<PAR>(in, out);
}
