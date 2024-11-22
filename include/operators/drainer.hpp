#ifndef __DRAINER_HPP__
#define __DRAINER_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename index_t,
    typename functor_t,
    typename stream_in_t,
    typename... Args
>
void Drainer (
    stream_in_t & stream_in,
    Args&&... args
)
{
    using input_t = typename stream_in_t::data_t;

    index_t index = 0;
    functor_t func(std::forward<Args>(args)...);

    bool last = stream_in.read_eos();
    
    Drainer:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        input_t in = stream_in.read();
        last = stream_in.read_eos();
        func(index, in, last);
        
        index++;
    }
}

}

#endif // __DRAINER_HPP__
