#ifndef __FILTER_HPP__
#define __FILTER_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename FUNCTOR_T,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename... Args
>
void Filter(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    Args&&... args
)
{
    using T_IN  = typename STREAM_IN::data_t;
    using T_OUT = typename STREAM_OUT::data_t;

    bool last = istrm.read_eos();

    FUNCTOR_T func(std::forward<Args>(args)...);

Filter:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out;
        bool flag = false;
        func(in, out, flag);
        if (flag) {
            ostrm.write(out);
        }
    }
    ostrm.write_eos();
}

}

#endif // __FILTER_HPP__
