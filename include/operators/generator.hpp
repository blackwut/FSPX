#ifndef __GENERATOR_HPP__
#define __GENERATOR_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"

// TODO: Args are not used in the A2A connectors

namespace fx {

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    typename STREAM_OUT,
    typename... Args
>
struct Generator
{
    void operator()(
        STREAM_OUT & ostrm,
        Args&&... args
    )
    {
        using T_OUT = typename STREAM_OUT::data_t;

        FUNCTOR_T func(std::forward<Args>(args)...);

        bool last = false;
        INDEX_T index = 0;
    Generator:
        while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
            T_OUT out;
            func(index, out, last);
            ostrm.write(out);
            ++index;
        }
        ostrm.write_eos();
    }
};

}

#endif // __GENERATOR_HPP__
