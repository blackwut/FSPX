#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__

#include "../../../../include/fspx.hpp"

#define IN_PAR      2
#define OUT_PAR     3
#define SIZE        8 * OUT_PAR


struct data_t
{
    int key;
    int val;

    bool operator==(const data_t& rhs) const
    {
        return (key == rhs.key) && (val == rhs.val);
    }

    bool operator!=(const data_t& rhs) const
    {
        return !operator==(rhs);
    }
};

using stream_t = fx::stream<data_t, 3>;


void test(
    stream_t in[IN_PAR],
    stream_t out[OUT_PAR]
);

#endif // __KERNEL_HPP__