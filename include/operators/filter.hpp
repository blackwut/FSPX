#ifndef __FILTER_HPP__
#define __FILTER_HPP__


#include "../common.hpp"
#include "../streams/streams.hpp"


namespace fx {


template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Filter(
    fx::stream<T_IN, DEPTH_IN> & istrm,
    fx::stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = istrm.read_eos();

Filter_StoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out;
        if (func(in, out)) {
            ostrm.write(out);
        }
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Filter(
    fx::axis_stream<T_IN, DEPTH_IN> & istrm,
    fx::stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = false;
    T_IN in = istrm.read(last);

Filter_AtoS:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_OUT out;
        if (func(in, out)) {
            ostrm.write(out);
        }

        in = istrm.read(last);
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Filter(
    fx::stream<T_IN, DEPTH_IN> & istrm,
    fx::axis_stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = istrm.read_eos();

Filter_StoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_IN in = istrm.read();
        last = istrm.read_eos();

        T_OUT out;
        if (func(in, out)) {
            ostrm.write(out);
        }
    }
    ostrm.write_eos();
}

template <typename T_IN, int DEPTH_IN, typename T_OUT, int DEPTH_OUT, typename FUNCTOR_T>
void Filter(
    fx::axis_stream<T_IN, DEPTH_IN> & istrm,
    fx::axis_stream<T_OUT, DEPTH_OUT> & ostrm,
    FUNCTOR_T && func
)
{
    bool last = false;
    T_IN in = istrm.read(last);

Filter_AtoA:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T_OUT out;
        if (func(in, out)) {
            ostrm.write(out);
        }

        in = istrm.read(last);
    }
    ostrm.write_eos();
}

// template <typename T_FUNC, typename T_IN, typename T_OUT = T_IN>
// void Filter(
//     T_FUNC & func,
//     hls::stream<T_IN> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T_OUT> & ostrm,
//     hls::stream<bool> & e_ostrm
// )
// {
//     bool last = e_istrm.read();

// MAIN_LOOP_FILTER:
//     while (!last) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        // T_IN in = istrm.read();
        // T_OUT out;
        // bool result = func(in, out);

        // if (result) {
        //     ostrm.write(out);
        //     e_ostrm.write(false);
        // }

//         last = e_istrm.read();
//     }
//     e_ostrm.write(true);
// }


// template <typename T_FUNC, typename T_IN, typename T_OUT = T_IN>
// void Filter(
//     T_FUNC & func,
//     fx::stream<T_IN> & istrm,
//     fx::stream<T_OUT> & ostrm
// )
// {
// #pragma HLS INLINE
//     Filter(
//         func,
//         istrm.data, istrm.e_data,
//         ostrm.data, ostrm.e_data
//     );
// }


// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_filter(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out,
//     T_FUNC func_array[N]
// )
// {
// #pragma HLS dataflow

// PROCESS_FILTER:
//     for (int i = 0; i < N; ++i) {
//     #pragma HLS unroll
//         Filter<T_FUNC, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void process_filter(
//     StreamN<T_IN, N, in_depth> & in,
//     StreamN<T_OUT, N, out_depth> & out
// )
// {
// #pragma HLS dataflow

//     T_FUNC func_array[N];

// PROCESS_FILTER:
//     for (int i = 0; i < N; ++i) {
// #pragma HLS unroll
//         Filter<T_FUNC, T_IN, T_OUT>(
//             func_array[i],
//             in.data[i], in.e_data[i],
//             out.data[i], out.e_data[i]
//         );
//     }
// }



// template <int N, typename T_FUNC, typename T_IN, typename T_OUT = T_IN, int in_depth, int out_depth>
// void Stream_to_Filter_Tag(
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

//     process_filter<N, T_FUNC, T_IN, T_OUT>(
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
// void Stream_to_Filter_Tag(
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

//     process_filter<N, T_FUNC, T_IN, T_OUT>(
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

#endif // __FILTER_HPP__
