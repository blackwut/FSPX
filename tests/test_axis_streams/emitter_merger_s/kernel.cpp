#include "kernel.hpp"

void test(
    stream_t & in,
    axis_stream_t internal_streams[PAR]
)
{
    fx::StoAN_RR<PAR>(in, internal_streams);
}

// void test(
//     axis_stream_t internal_streams[PAR],
//     stream_t & out
// )
// {
//     fx::ANtoS_RR<PAR>(internal_streams, out);
// }
