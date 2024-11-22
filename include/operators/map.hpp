#ifndef __MAP_HPP__
#define __MAP_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename functor_t,
    typename stream_in_t,
    typename stream_out_t,
    typename... Args
>
void Map (
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    Args&&... args
)
{
    using input_t  = typename stream_in_t::data_t;
    using output_t = typename stream_out_t::data_t;

    functor_t func(std::forward<Args>(args)...);

    bool last = stream_in.read_eos();

    Map:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        input_t in = stream_in.read();
        last = stream_in.read_eos();

        output_t out;
        func(in, out);

        stream_out.write(out);
    }
    stream_out.write_eos();
}

}

#endif // __MAP_HPP__
