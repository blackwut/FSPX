#ifndef __CONSTANTS_HPP__
#define __CONSTANTS_HPP__

// #include "fspx.hpp"
#include "tuple.hpp"

constexpr int K = 2;                                // number of kernel calls
constexpr int SIZE = 512;                           // input size
constexpr int WIDTH = 512;                          // width of bus
constexpr int ITEM_BITS = sizeof(record_t) * 8;     // number of bits of an item
constexpr int READ_WRITE_ITEMS = WIDTH / ITEM_BITS; // number of items in a read/write operation

using line_t = ap_uint<WIDTH>;                      // read/write datatype
using axis_stream_t = fx::axis_stream<record_t, 2>;


constexpr int SO_PAR = 1;
constexpr int MA_PAR = 2;
constexpr int SD_PAR = 2;
constexpr int SI_PAR = 1;

#endif // __CONSTANTS_HPP__