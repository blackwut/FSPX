#ifndef __AXIS_STREAM_HPP__
#define __AXIS_STREAM_HPP__

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "../common.hpp"


namespace fx {

// enum class Storage {
//     Unspecified,  // Let the tool decide
//     BRAM,
//     LUTRAM,
//     SRL
// };

template <typename T, int depth = 2>
struct axis_stream
{
    using wrapper_t = hls::axis<T, 0, 0, 0>;

    hls::stream<wrapper_t> data;

    axis_stream() {}

    axis_stream(const char * name) {
        data.set_name(name);
    }

    T read(bool & eos)
    {
    #pragma HLS INLINE
        wrapper_t w = data.read();
        eos = w.last;
        return w.data;
    }

    void write(const T & v)
    {
    #pragma HLS INLINE
        wrapper_t w;
        w.data = v;
        w.keep = -1;
        w.last = false;
        data.write(w);
    }

    void write_eos()
    {
    #pragma HLS INLINE
        wrapper_t w;
        w.keep = -1;
        w.last = false;
        data.write(w);
    }

    bool empty()
    {
    #pragma HLS INLINE
        return data.empty();
    }

    bool full()
    {
    #pragma HLS INLINE
        return data.full();
    }
};

}

#endif // __AXIS_STREAM_HPP__
