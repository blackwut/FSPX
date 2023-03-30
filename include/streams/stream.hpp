#ifndef __STREAM_HPP__
#define __STREAM_HPP__

#include "hls_stream.h"
#include "../common.hpp"


namespace fx {

// enum class Storage {
//     Unspecified,  // Let the tool decide
//     BRAM,
//     LUTRAM,
//     SRL
// };

template <typename T, int depth = 2>
struct stream
{
    hls::stream<T> data;
    hls::stream<bool> e_data;

    stream() {
        #pragma HLS stream variable=data   depth=depth
        #pragma HLS stream variable=e_data depth=depth
    }

    stream(const char * name) {
        #pragma HLS stream variable=data   depth=depth
        #pragma HLS stream variable=e_data depth=depth
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

    bool full()
    {
    #pragma HLS INLINE
        return data.full();
    }
};

}

#endif // __STREAM_HPP__
