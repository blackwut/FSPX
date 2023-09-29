#ifndef __DRAINER_HPP__
#define __DRAINER_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <
    typename INDEX_T,
    typename FUNCTOR_T,
    typename STREAM_IN,
    typename... Args
>
void Drainer(
    STREAM_IN & istrm,
    Args&&... args
)
{
    using T_IN = typename STREAM_IN::data_t;

    FUNCTOR_T func(std::forward<Args>(args)...);

    bool last = istrm.read_eos();
    INDEX_T index = 0;
Drainer:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();
        func(index, in, last);
        ++index;
    }
}

}

#endif // __DRAINER_HPP__
