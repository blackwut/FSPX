#include "../../../include/fspx.hpp"

struct data_t
{
    char x[4];
    short y[2];
    double z;

    bool operator==(const data_t& rhs) const
    {
        return (x[0] == rhs.x[0])
            && (x[0] == rhs.x[0])
            && (x[1] == rhs.x[1])
            && (x[2] == rhs.x[2])
            && (x[3] == rhs.x[3])
            && (y[0] == rhs.y[0])
            && (y[1] == rhs.y[1])
            && (z == rhs.z);
    }

    bool operator!=(const data_t& rhs) const
    {
        return !operator==(rhs);
    }
};


constexpr int PAR = 4;                          // parallelism degree
constexpr int SIZE = 16;                        // input size
constexpr int WIDTH = 512;                      // width of bus
constexpr int ITEM_BITS = sizeof(data_t) * 8;   // number of bits of an item
constexpr int READ_ITEMS = WIDTH / ITEM_BITS;   // number of items in a read operation


using line_t = ap_uint<WIDTH>;                  // read/write datatype
using stream_t = fx::stream<data_t, 2>;


void test(
    line_t * in,
    stream_t out[READ_ITEMS],
    int number_of_lines
);
