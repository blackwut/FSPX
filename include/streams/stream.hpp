#ifndef __STREAMS_STREAM_HPP__
#define __STREAMS_STREAM_HPP__

#pragma GCC system_header
#include "hls_stream.h"
#include "../common.hpp"


namespace fx {

template <typename T, int DEPTH = 2>
using stream_single = hls::stream<T, DEPTH>;


template <typename T, int DEPTH = 2>
struct stream
{
    using data_t = T;

    hls::stream<T> data;
    hls::stream<bool> e_data;

    stream() {
        #pragma HLS STREAM variable=data   depth=DEPTH
        #pragma HLS STREAM variable=e_data depth=DEPTH
    }

    stream(const char * name)
    : stream<T, DEPTH>() {
        data.set_name(name);
        e_data.set_name(name);
    }

    T read()
    {
    #pragma HLS INLINE
        return data.read();
    }

    void write(const T & v)
    {
    #pragma HLS INLINE
        data.write(v);
        e_data.write(false);
    }

    bool read_eos()
    {
    #pragma HLS INLINE
        return e_data.read();
    }

    void write_eos()
    {
    #pragma HLS INLINE
        e_data.write(true);
    }

    bool empty()
    {
    #pragma HLS INLINE
        return data.empty();
    }

    bool empty_eos()
    {
    #pragma HLS INLINE
        return e_data.empty();
    }

    bool full()
    {
    #pragma HLS INLINE
        return data.full();
    }

    bool full_eos()
    {
    #pragma HLS INLINE
        return e_data.full();
    }
};

}

#endif // __STREAMS_STREAM_HPP__
