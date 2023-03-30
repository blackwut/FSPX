#ifndef __FLATMAP_HPP__
#define __FLATMAP_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {


template <typename T_OUT>
struct FlatMapShipper
{
    hls::stream<T_OUT> & data;
    hls::stream<bool> & e_data;

    FlatMapShipper(
        hls::stream<T_OUT> & data,
        hls::stream<bool> & e_data
    )
    : data(data)
    , e_data(e_data)
    {}

    void send(T_OUT & out)
    {
    #pragma HLS INLINE
        data.write(out);
        e_data.write(false);
    }
};


template <typename T_FUNC, typename T_IN, typename T_OUT = T_IN>
void FlatMap(
    T_FUNC & func,
    hls::stream<T_IN> & istrm,
    hls::stream<bool> & e_istrm,
    hls::stream<T_OUT> & ostrm,
    hls::stream<bool> & e_ostrm
)
{
    static FlatMapShipper<T_OUT> shipper(ostrm, e_ostrm);

    bool last = e_istrm.read();

MAIN_LOOP_FLATMAP:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        func(in, shipper);

        last = e_istrm.read();
    }
    e_ostrm.write(true);
}


template <typename T_FUNC, typename T_IN, typename T_OUT = T_IN>
void FlatMap(
    T_FUNC & func,
    fx::stream<T_IN> & istrm,
    fx::stream<T_OUT> & ostrm
)
{
#pragma HLS INLINE
    FlatMap(
        func,
        istrm.data, istrm.e_data,
        ostrm.data, ostrm.e_data
    );
}


// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_flatmap(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out,
//     T_FUNC func_array[N]
// )
// {
// #pragma HLS dataflow

// PROCESS_FLATMAP:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         FlatMap<T_FUNC, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_flatmap(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out
// )
// {
// #pragma HLS dataflow

//     T_FUNC func_array[N];

// PROCESS_FLATMAP:
//     for (int i = 0; i < N; ++i) {
// #pragma HLS unroll
//         FlatMap<T_FUNC, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void Stream_to_FlatMap_Tag(
//     Stream<T_IN, in_depth> & in,
//     Stream<T_OUT, out_depth> & out
// )
// {
// #pragma HLS dataflow

//     StreamTag<T_IN, 16> stream_to_tag;
//     StreamN<T_IN, N, 16> tag_to_mpu;
//     StreamN<T_OUT, N, 16> mpu_to_stream;

//     Stream_to_StreamTag<T_IN, N>(
//         in, stream_to_tag
//     );

//     xf::common::utils_hw::streamOneToN<T_IN, TAG_T<T_IN>::width, N>(
//         stream_to_tag.data, stream_to_tag.e_data,
//         stream_to_tag.tag, stream_to_tag.e_tag,
//         tag_to_mpu.data, tag_to_mpu.e_data,
//         xf::common::utils_hw::TagSelectT()
//     );

//     process_flatmap<N, T_FUNC, T_IN, T_OUT>(
//         tag_to_mpu,
//         mpu_to_stream
//     );

//     xf::common::utils_hw::streamNToOne<T_OUT, N>(
//         mpu_to_stream.data, mpu_to_stream.e_data,
//         out.data, out.e_data,
//         xf::common::utils_hw::LoadBalanceT()
//     );
// }



// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void Stream_to_FlatMap_Tag(
//     Stream<T_IN, in_depth> & in,
//     Stream<T_OUT, out_depth> & out,
//     T_FUNC func_array[N]
// )
// {
// #pragma HLS dataflow

//     StreamTag<T_IN, 16> stream_to_tag;
//     StreamN<T_IN, N, 16> tag_to_mpu;
//     StreamN<T_OUT, N, 16> mpu_to_stream;

//     Stream_to_StreamTag<T_IN, N>(
//         in, stream_to_tag
//     );

//     xf::common::utils_hw::streamOneToN<T_IN, TAG_T<T_IN>::width, N>(
//         stream_to_tag.data, stream_to_tag.e_data,
//         stream_to_tag.tag, stream_to_tag.e_tag,
//         tag_to_mpu.data, tag_to_mpu.e_data,
//         xf::common::utils_hw::TagSelectT()
//     );

//     process_flatmap<N, T_FUNC, T_IN, T_OUT>(
//         tag_to_mpu,
//         mpu_to_stream,
//         func_array
//     );

//     xf::common::utils_hw::streamNToOne<T_OUT, N>(
//         mpu_to_stream.data, mpu_to_stream.e_data,
//         out.data, out.e_data,
//         xf::common::utils_hw::LoadBalanceT()
//     );
// }

}

#endif // __FLATMAP_HPP__
