#include "kernel.hpp"

void test(
    stream_t & in,
    axis_stream_t out[PAR]
)
{
    fx::StoAN_RR<PAR>(in, out);
}
