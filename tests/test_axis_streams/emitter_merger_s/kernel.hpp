#include "../../../include/fspx.hpp"

#define PAR      4
#define SIZE    16

using data_t = int;
using stream_t = fx::stream<data_t, 2>;
using axis_stream_t = fx::axis_stream<data_t>;

void test(
    stream_t & in,
    axis_stream_t internal_streams[PAR]
);

// void test(
//     axis_stream_t internal_streams[PAR],
//     stream_t & out
// );