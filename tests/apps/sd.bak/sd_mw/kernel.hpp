#include "../../../../include/fspx.hpp"

#include "../common/utils.hpp"
#include "../common/tuple.hpp"
#include "../common/constants.hpp"
#include "../common/moving_average.hpp"
#include "../common/spike_detector.hpp"

void test(
    axis_stream_t & in,
    line_t * out,
    int out_size,
    int * write_count,
    int * eos
);
