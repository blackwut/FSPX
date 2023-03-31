#ifndef __MAP_HPP__
#define __MAP_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {


template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Map(
    fx::stream<T_IN, DEPTH_IN> & istrm,
    fx::stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = istrm.read_eos();

Map_StoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out = func(in);

        ostrm.write(out);
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Map(
    fx::axis_stream<T_IN, DEPTH_IN> & istrm,
    fx::stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = false;
    T_IN in = istrm.read(last);

Map_AtoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_OUT out = func(in);
        ostrm.write(out);

        in = istrm.read(last);
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Map(
    fx::stream<T_IN, DEPTH_IN> & istrm,
    fx::axis_stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = istrm.read_eos();

Map_StoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out = func(in);

        ostrm.write(out);
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Map(
    fx::axis_stream<T_IN, DEPTH_IN> & istrm,
    fx::axis_stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = false;
    T_IN in = istrm.read(last);

Map_AtoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_OUT out = func(in);
        ostrm.write(out);

        in = istrm.read(last);
    }
    ostrm.write_eos();
}


// template <typename FUNCTOR_T, typename T_IN, typename T_OUT = T_IN>
// void Map(
//     FUNCTOR_T & func,
//     fx::stream<T_IN> & istrm,
//     fx::stream<T_OUT> & ostrm
// )
// {
// #pragma HLS INLINE
//     Map(
//         func,
//         istrm.data, istrm.e_data,
//         ostrm.data, ostrm.e_data
//     );
// }


// template <int N, typename FUNCTOR_T, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_map(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out,
//     FUNCTOR_T func_array[N]
// )
// {
// #pragma HLS dataflow

// PROCESS_MAP:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         Map<FUNCTOR_T, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename FUNCTOR_T, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_map(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out
// )
// {
// #pragma HLS dataflow

//     FUNCTOR_T func_array[N];

// PROCESS_MAP:
//     for (int i = 0; i < N; ++i) {
// #pragma HLS unroll
//         Map<FUNCTOR_T, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename FUNCTOR_T, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void Stream_to_Map_Tag(
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

//     process_map<N, FUNCTOR_T, T_IN, T_OUT>(
//         tag_to_mpu,
//         mpu_to_stream
//     );

//     xf::common::utils_hw::streamNToOne<T_OUT, N>(
//         mpu_to_stream.data, mpu_to_stream.e_data,
//         out.data, out.e_data,
//         xf::common::utils_hw::LoadBalanceT()
//     );
// }



// template <int N, typename FUNCTOR_T, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void Stream_to_Map_Tag(
//     Stream<T_IN, in_depth> & in,
//     Stream<T_OUT, out_depth> & out,
//     FUNCTOR_T func_array[N]
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

//     process_map<N, FUNCTOR_T, T_IN, T_OUT>(
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

#endif // __MAP_HPP__
