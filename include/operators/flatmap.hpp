#ifndef __FLATMAP_HPP__
#define __FLATMAP_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"

// TODO: Args are not used in the A2A connectors

namespace fx {

template <typename STREAM_OUT>
struct FlatMapShipper
{
    using T_OUT = typename STREAM_OUT::data_t;

    STREAM_OUT & ostrm;

    FlatMapShipper(
        STREAM_OUT & ostrm
    )
    : ostrm(ostrm)
    {
    #pragma HLS INLINE
    }

    void send(T_OUT & out)
    {
    #pragma HLS INLINE
        ostrm.write(out);
    }

    void send_eos()
    {
    #pragma HLS INLINE
        ostrm.write_eos();
    }
};


template <
    typename FUNCTOR_T,
    typename STREAM_IN,
    typename STREAM_OUT,
    typename... Args
>
void FlatMap(
    STREAM_IN & istrm,
    STREAM_OUT & ostrm,
    Args&&... args
)
{
    using T_IN  = typename STREAM_IN::data_t;
    using T_OUT = typename STREAM_OUT::data_t;

    static FUNCTOR_T func(std::forward<Args>(args)...);
    static FlatMapShipper<STREAM_OUT> shipper(ostrm);

    bool last = istrm.read_eos();

FlatMap:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        func(in, shipper);
    }
    shipper.send_eos();
}

}

#endif // __FLATMAP_HPP__
