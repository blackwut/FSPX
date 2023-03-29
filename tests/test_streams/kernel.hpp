#include "../../include/fspx.hpp"

#define N       4       // number of output streams
#define SIZE    16      // size of the input stream

using data_t = int;
using stream_t = fx::stream<data_t>;

enum TestCase : int {
    StoS,
    RoundRobin,
    LoadBalancer,
    KeyBy,
    Broadcast
};

void test(
    stream_t & in,
    stream_t & out,
    TestCase p
);
