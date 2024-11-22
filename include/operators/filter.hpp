#ifndef __FILTER_HPP__
#define __FILTER_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename functor_t,
    typename stream_in_t,
    typename stream_out_t,
    typename... Args
>
void Filter(
    stream_in_t & istrm,
    stream_out_t & ostrm,
    Args&&... args
)
{
    #pragma HLS INLINE recursive // TODO: check if this is needed
    
    using input_t  = typename stream_in_t::data_t;
    using output_t = typename stream_out_t::data_t;

    functor_t func(std::forward<Args>(args)...);

    bool last = istrm.read_eos();

    Filter:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        input_t in = istrm.read();
        last = istrm.read_eos();

        output_t out;
        bool flag = false;
        func(in, out, flag);
        if (flag) {
            ostrm.write(out);
        }
    }

    ostrm.write_eos();
}

} // namespace fx

#endif // __FILTER_HPP__
