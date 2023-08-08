#ifndef __MAP_HPP__
#define __MAP_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"

// TODO: Args are not used in the A2A connectors

namespace fx {

template <
    typename FUNCTOR_T,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename... Args
>
void Map(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    Args&&... args
)
{
    using T_IN  = typename STREAM_IN::data_t;
    using T_OUT = typename STREAM_OUT::data_t;

    bool last = istrm.read_eos();

    static FUNCTOR_T func(std::forward<Args>(args)...);

Map:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out;
        func(in, out);

        ostrm.write(out);
    }
    ostrm.write_eos();
}

}

#endif // __MAP_HPP__
