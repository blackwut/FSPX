#include "../../include/fspx.hpp"

// #define MULTISTREAM // COSIM not working!!!
// #define COMPACTOR
// #define INSERT_SORT
// #define LATE_TIME_TUMBLING_WINDOW_OPERATOR
#define KEYED_LATE_TIME_TUMBLING_WINDOW_OPERATOR
// #define LATE_TIME_TUMBLING_WINDOW
// #define KEYED_TIME_TUMBLING_WINDOW
// #define TIME_TUMBLING_WINDOW
// #define KEYED_COUNT_TUMBLING_WINDOW

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
static constexpr int WINDOW_LATENESS = 3;

static constexpr unsigned int MAX_KEYS = 16;
// static constexpr int DATA_SIZE = MAX_KEYS * (WINDOW_SIZE + ((WINDOW_LATENESS + WINDOW_SIZE - 1) / WINDOW_SIZE)) * 5 + 1;
static constexpr int DATA_SIZE = 64;

using in_stream_t = fx::axis_stream<data_t, 2>;
using out_stream_t = fx::axis_stream<data_t, 2>;

void test(
    in_stream_t & in,
    out_stream_t & out
);
