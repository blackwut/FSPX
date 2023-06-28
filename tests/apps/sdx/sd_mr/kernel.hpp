#include "../../../../include/fspx.hpp"

#include "../common/utils.hpp"
#include "../common/tuple.hpp"
#include "../common/constants.hpp"
#include "../common/moving_average.hpp"
#include "../common/spike_detector.hpp"

void test(
    line_t * in,
    int in_count,
    int eos,
    axis_stream_t & out
);
