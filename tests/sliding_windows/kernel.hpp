#include "../../include/fspx.hpp"

struct data_t {
    unsigned int key;
    float value;
    float aggregate;
    unsigned int timestamp;

    data_t() = default;

    data_t(unsigned int key, float value, float aggregate, unsigned int timestamp)
        : key(key), value(value), aggregate(aggregate), timestamp(timestamp)
    {}

    #if defined(SYNTHESIS)
    friend std::ostream & operator<<(std::ostream & os, const data_t & d)
    {
        os << "(key: " << d.key << ", value: " << d.value << ", aggregate: " << d.aggregate << ", timestamp: " << d.timestamp << ")";
        return os;
    }
    #endif
};

static constexpr int WINDOW_SIZE = 1;
static constexpr int WINDOW_STEP = 1;
static constexpr int WINDOW_LATENESS = 0;

static constexpr unsigned int MAX_KEYS = 2;
// static constexpr int DATA_SIZE = MAX_KEYS * (WINDOW_SIZE + ((WINDOW_LATENESS + WINDOW_SIZE - 1) / WINDOW_SIZE)) * 5 + 1;
static constexpr int DATA_SIZE = 64;

using in_stream_t = fx::axis_stream<data_t, 32>;
using out_stream_t = fx::axis_stream<data_t, 32>;

void test(
    in_stream_t & in,
    out_stream_t & out
);
