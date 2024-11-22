#ifndef __GENERATOR_HPP__
#define __GENERATOR_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename index_t,
    typename functor_t,
    typename stream_out_t,
    typename... Args
>
void Generator(
    stream_out_t & ostrm,
    Args&&... args
)
{
    using output_t = typename stream_out_t::data_t;

    index_t index = 0;
    functor_t func(std::forward<Args>(args)...);

    bool last = false;

    Generator:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        output_t out;
        func(index, out, last);
        ostrm.write(out);
        index++;
    }
    ostrm.write_eos();
}

}

#endif // __GENERATOR_HPP__
