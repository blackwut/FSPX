#ifndef __AXIS_STREAM_HPP__
#define __AXIS_STREAM_HPP__

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "../common.hpp"


namespace fx {

template <typename T, int DEPTH = 2>
struct axis_stream
{
    using data_t = T;
    using wdata_t = hls::axis<T, 0, 0, 0>;
    using weos_t = hls::axis<bool, 0, 0, 0>;

    hls::stream<wdata_t> data;
    hls::stream<weos_t> e_data;

    axis_stream() {
        #pragma HLS INTERFACE mode=axis port=data
        #pragma HLS INTERFACE mode=axis port=e_data
    }

    axis_stream(const char * name)
    : axis_stream<T, DEPTH>() {
        data.set_name(name);
        // e_data.set_name(name);
    }

    T read()
    {
    #pragma HLS INLINE
        wdata_t d = data.read();
        return d.data;
    }

    void write(const T & v)
    {
    #pragma HLS INLINE
        wdata_t d;
        d.data = v;
        d.keep = -1;
        data.write(d);

        weos_t e;
        e.data = false;
        e.keep = -1;
        e_data.write(e);
    }

    bool read_eos()
    {
    #pragma HLS INLINE
        weos_t e = e_data.read();
        return e.data;
    }

    void write_eos()
    {
    #pragma HLS INLINE
        weos_t e;
        e.data = true;
        e.keep = -1;
        e_data.write(e);
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
