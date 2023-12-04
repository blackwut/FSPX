#include "../../include/fspx.hpp"

struct data_t {
    int key;
    float value;
    float aggregate;
    int padding;
};

static constexpr int DATA_SIZE = 32;
static constexpr int MAX_KEYS = 4;

static constexpr int WINDOW_SIZE = 16;
static constexpr int WINDOW_STEP = 7;

using in_stream_t = fx::axis_stream<data_t, 2>;
using out_stream_t = fx::axis_stream<data_t, 2>;

void test(
    in_stream_t & in,
    out_stream_t & out
);
