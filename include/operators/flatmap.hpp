#ifndef __FLATMAP_HPP__
#define __FLATMAP_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {

template <typename stream_out_t>
struct Shipper
{
    using data_t = typename stream_out_t::data_t;

    stream_out_t & stream_out;

    Shipper(stream_out_t & stream_out)
    : stream_out(stream_out)
    {}

    void send(const data_t & out) // TODO: check if const can cause issues
    {
        #pragma HLS INLINE
        stream_out.write(out);
    }

    void send_eos()
    {
        #pragma HLS INLINE
        stream_out.write_eos();
    }
};

template <
    typename functor_t,
    size_t LATENCY = 1,
    typename stream_in_t,
    typename stream_out_t,
    typename... Args
>
void FlatMap(
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    Args&&... args
)
{
    using input_t = typename stream_in_t::data_t;

    Shipper<stream_out_t> shipper(stream_out);
    functor_t func(std::forward<Args>(args)...);

    bool last = stream_in.read_eos();

    FlatMap:
    while (!last) {
        #pragma HLS PIPELINE II = LATENCY
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        input_t in = stream_in.read();
        last = stream_in.read_eos();
        func(in, shipper);
    }

    shipper.send_eos();
}

template <
    typename stream_out_t,
    unsigned int SIZE
>
struct ParallelShipper
{
    using data_t = typename stream_out_t::data_t;

    stream_out_t (&streams_out)[SIZE];

    ParallelShipper(stream_out_t (&streams_out)[SIZE])
    : streams_out(streams_out)
    {}

    void send(
        const unsigned int i,
        const data_t & out
    )
    {
        #pragma HLS INLINE
        streams_out[i].write(out);
    }

    void send_eos(const unsigned int i)
    {
        #pragma HLS INLINE
        streams_out[i].write_eos();
    }

    void send_all(const data_t & out)
    {
        #pragma HLS INLINE
        for (unsigned int i = 0; i < SIZE; ++i) {
            #pragma HLS UNROLL
            
            streams_out[i].write(out);
        }
    }

    void send_all_eos()
    {
        #pragma HLS INLINE
        for (unsigned int i = 0; i < SIZE; ++i) {
            #pragma HLS UNROLL
            
            streams_out[i].write_eos();
        }
    }
};

template <
    typename functor_t,
    unsigned int SIZE,
    typename stream_in_t,
    typename stream_out_t,
    typename... Args
>
void ParallelFlatMap(
    stream_in_t & stream_in,
    stream_out_t (&ostrms)[SIZE],
    Args&&... args
)
{
    using input_t = typename stream_in_t::data_t;

    ParallelShipper<stream_out_t, SIZE> shipper(ostrms);
    functor_t func(std::forward<Args>(args)...);

    bool last = stream_in.read_eos();

    ParallelFlatMap:
    while (!last) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        
        input_t in = stream_in.read();
        last = stream_in.read_eos();
        func(in, shipper);
    }

    shipper.send_all_eos();
}

}

#endif // __FLATMAP_HPP__
