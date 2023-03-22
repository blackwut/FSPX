#ifndef __STREAM_HPP__
#define __STREAM_HPP__


namespace fx {
// Constants for axi functions
// constexpr auto BURST_LENGTH = 32;
// constexpr auto W_AXI = 512;

// enum class Storage {
//     Unspecified,  // Let the tool decide
//     BRAM,
//     LUTRAM,
//     SRL
// };

// template <typename T>
// struct TAG_T
// {
//     static constexpr int width = xf::common::utils_hw::details::Log2<T::MAX_KEY_VALUE>::value;
//     using type = ap_uint<width>;
// };



template <typename T, int depth = 2>
struct stream
{
    hls::stream<T> data;
    hls::stream<bool> e_data;

    stream() {
        #pragma HLS STREAM variable=data   depth=depth
        #pragma HLS STREAM variable=e_data depth=depth
    }
};



// template <typename T, int N, int depth = 2>
// struct StreamN
// {
//     hls::stream<T> data[N];
//     hls::stream<bool> e_data[N];

//     StreamN() {
//         #pragma HLS STREAM variable=data   depth=depth
//         #pragma HLS STREAM variable=e_data depth=depth
//     }
// };



// template <typename T, int depth = 2>
// struct StreamTag
// {
//     hls::stream<T> data;
//     hls::stream<bool> e_data;
//     hls::stream<typename TAG_T<T>::type> tag;
//     hls::stream<bool> e_tag;

//     StreamTag() {
//         #pragma HLS STREAM variable=data   depth=depth
//         #pragma HLS STREAM variable=e_data depth=depth
//         #pragma HLS STREAM variable=tag    depth=depth
//         #pragma HLS STREAM variable=e_tag  depth=depth
//     }
// };



// template <typename T, int _WStrm, int in_depth, int out_depth>
// void StreamT_to_StreamAP(
//     Stream<T, in_depth> & in,
//     Stream<ap_uint<_WStrm>, out_depth> & out
// )
// {
//     bool last = in.e_data.read();
// STREAM_T_TO_STREAM_AP:
//     while (!last) {
//     #pragma HLS pipeline II = 1
//     #pragma HLS loop_tripcount min = 1 max = 128
//         T t = in.data.read();

//         out.data.write((ap_uint<_WStrm>)t);
//         out.e_data.write(false);
        
//         last = in.e_data.read();
//     }
//     out.e_data.write(true);
// }



// template <typename T, int depth>
// void Mem_to_Stream(
//     ap_uint<W_AXI> * data,
//     const int size,
//     Stream<T, depth> & stream
// )
// {
// #pragma HLS DATAFLOW
//     xf::common::utils_hw::axiToStream<BURST_LENGTH, W_AXI, T> (
//         data, size,
//         stream.data, stream.e_data
//     );
// }

// template <typename T>
// void Mem_to_Stream(
//     ap_uint<W_AXI> * data,
//     const int size,
//     hls::stream<T> & out,
//     hls::stream<bool> & e_out
// )
// {
// #pragma HLS DATAFLOW
//     xf::common::utils_hw::axiToStream<BURST_LENGTH, W_AXI, T> (
//         data, size,
//         out, e_out
//     );
// }



// template <typename T, int depth>
// void Stream_to_Mem(
//     ap_uint<W_AXI> * data,
//     Stream<T, depth> & stream
// )
// {
//     const int _WStrm = sizeof(T) * 8;
//     Stream<ap_uint<_WStrm>, 16> s_to_ap;

// #pragma HLS DATAFLOW
//     StreamT_to_StreamAP<T, _WStrm>(stream, s_to_ap);

//     xf::common::utils_hw::streamToAxi<BURST_LENGTH, W_AXI, _WStrm> (
//         data,
//         s_to_ap.data, s_to_ap.e_data
//     );
// }



// template <typename T_IN>
// void CountResults(
//     hls::stream<T_IN> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T_IN> & ostrm,
//     hls::stream<bool> & e_ostrm,
//     int * count
// )
// {
//     static int count_ = 0;
//     bool last = e_istrm.read();

// MAIN_LOOP_COUNT_RESULTS:
//     while (!last) {
//     #pragma HLS pipeline II = 1
//     #pragma HLS loop_tripcount min = 1 max = 128
//         T_IN in = istrm.read();

//         count_++;

//         ostrm.write(in);
//         e_ostrm.write(false);
//         last = e_istrm.read();
//     }
//     e_ostrm.write(true);

//     count[0] = count_;
// }

// template <typename T_IN>
// void CountResults(
//     hls::stream<T_IN> & istrm,
//     hls::stream<bool> & e_istrm,
//     hls::stream<T_IN> & ostrm,
//     hls::stream<bool> & e_ostrm,
//     int * count,
//     int max
// )
// {
//     static int count_ = 0;
//     bool last = e_istrm.read();

// MAIN_LOOP_COUNT_RESULTS:
//     while (!last && (count_ < max)) {
//     #pragma HLS pipeline II = 1
//     #pragma HLS loop_tripcount min = 1 max = 128
//         T_IN in = istrm.read();

//         count_++;

//         ostrm.write(in);
//         e_ostrm.write(false);
//         last = e_istrm.read();
//     }
//     e_ostrm.write(true);

//     count[0] = count_;
// }

// template <typename T, int depth>
// void Stream_to_Mem(
//     ap_uint<W_AXI> * data,
//     int * count,
//     Stream<T, depth> & stream
// )
// {
//     const int _WStrm = sizeof(T) * 8;
//     Stream<T, 16> count_to_s;
//     Stream<ap_uint<_WStrm>, 16> s_to_ap;

// #pragma HLS DATAFLOW

//     CountResults<T>(
//         stream.data, stream.e_data,
//         count_to_s.data, count_to_s.e_data,
//         count);
//     StreamT_to_StreamAP<T, _WStrm>(count_to_s, s_to_ap);

//     xf::common::utils_hw::streamToAxi<BURST_LENGTH, W_AXI, _WStrm> (
//         data,
//         s_to_ap.data, s_to_ap.e_data
//     );
// }

// template <typename T, int depth>
// void Stream_to_Mem(
//     ap_uint<W_AXI> * data,
//     int * count,
//     hls::stream<T> & in,
//     hls::stream<bool> & e_in,
//     int max
// )
// {
//     const int _WStrm = sizeof(T) * 8;
//     Stream<T, 16> count_to_s;
//     Stream<ap_uint<_WStrm>, 16> s_to_ap;

// #pragma HLS DATAFLOW

//     CountResults<T>(
//         in, e_in,
//         count_to_s.data, count_to_s.e_data,
//         count, max);
//     StreamT_to_StreamAP<T, _WStrm>(count_to_s, s_to_ap);

//     xf::common::utils_hw::streamToAxi<BURST_LENGTH, W_AXI, _WStrm> (
//         data,
//         s_to_ap.data, s_to_ap.e_data
//     );
// }


// template <typename T, int N, int in_depth, int out_depth>
// void Stream_to_StreamTag(
//     Stream<T, in_depth> & in,
//     StreamTag<T, out_depth> & out
// )
// {
//     bool last = in.e_data.read();
// STREAM_TO_STREAM_TAG:
//     while (!last) {
//     #pragma HLS pipeline II = 1
//     #pragma HLS loop_tripcount min = 1 max = 128
//         T t = in.data.read();

//         out.data.write(t);
//         out.e_data.write(false);

//         out.tag.write(t.getKey() % N);
//         out.e_tag.write(false);
        
//         last = in.e_data.read();
//     }
//     out.e_data.write(true);
//     out.e_tag.write(true);
// }

}

#endif // __STREAM_HPP__
