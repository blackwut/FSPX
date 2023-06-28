#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__

#include "../../../include/fspx.hpp"

#define SIZE    1024
#define PAR     4

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

using stream_t = fx::stream<data_t, 1>;
using in_stream_t = fx::stream<data_t, 2>;
using out_stream_t = fx::stream<data_t, 8>;

void test(
    stream_t & in,
    stream_t & out
);

#endif // __KERNEL_HPP__