#ifndef __DEFINES_HPP__
#define __DEFINES_HPP__

#include "fspx.hpp"
#include "tuples/record_t.hpp"

static constexpr int WIDTH = 512;
using line_t = ap_uint<WIDTH>;
using stream_t = fx::stream<record_t, 16>;
using axis_stream_t = fx::axis_stream<record_t, 16>;

// #define MR_PAR 2
// #define MA_PAR 2
// #define SD_PAR 2
// #define MW_PAR 1

#endif // __DEFINES_HPP__