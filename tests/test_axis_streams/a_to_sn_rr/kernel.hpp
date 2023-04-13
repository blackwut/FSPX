#include "../../../include/fspx.hpp"

#define PAR      4
#define SIZE     16

using data_t = int;
using stream_t = fx::stream<data_t, 2>;
using axis_stream_t = fx::axis_stream<data_t>;

void test(
    axis_stream_t & in,
    stream_t out[PAR]
);
