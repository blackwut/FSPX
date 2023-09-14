// #ifndef __CONNECTORS_MEMORY_HPP__
// #define __CONNECTORS_MEMORY_HPP__


// #include "ap_int.h"
// #include "../common.hpp"
// #include "../connectors/connectors.hpp"
// #include "../datastructures/datastructures.hpp"
// #include "../streams/streams.hpp"


// namespace fx {

// //******************************************************************************
// //
// // Memory to Stream
// //
// //******************************************************************************

// template <int W, typename STREAM_OUT>
// void WMtoWS(
//     ap_uint<W> * in,
//     int count,
//     bool eos,
//     STREAM_OUT & out
// )
// {
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

// WMtoWS:
//     for (int i = 0; i < count; ++i) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         out.write(in[i]);
//     }

//     if (eos) {
//         out.write_eos();
//     }
// }

// template <int W, typename STREAM_OUT>
// void WMtoS(
//     ap_uint<W> * in,
//     int count,
//     bool eos,
//     STREAM_OUT & out
// )
// {
//     using T = typename STREAM_OUT::data_t;

//     HW_STATIC_ASSERT(W % sizeof(T) == 0,
//                      "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     constexpr int T_BITS = sizeof(T) * 8;   // item size in bits
//     constexpr int READ_ITEMS = W / T_BITS;  // number of items in a read operation

// WMtoS:
//     for (int i = 0; i < count; ++i) {
//     #pragma HLS PIPELINE II = READ_ITEMS
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

//         DataPack<T, READ_ITEMS> line = in[i];

// WMtoS_write:
//         for (int j = 0; j < READ_ITEMS; ++j) {
//         #pragma HLS PIPELINE II = 1
//         #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//             out.write(line[j]);
//         }
//     }

//     if (eos) {
//         out.write_eos();
//     }
// }


// //******************************************************************************
// //
// // Stream to Memory
// //
// //******************************************************************************

// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void prepare_burst_wide(
//     STREAM_IN & in,
//     hls::stream< ap_uint<W> > & out,
//     hls::stream< ap_uint<8> > & burst_size,
//     hls::stream<bool> & eos_signal,
//     int out_size
// )
// {
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     const int WRITE_MAX_COUNT = out_size / (W / 8); // max number of write operations

//     int wc = 0;   // count the total number of write operations
//     int bc = 0;   // count the number of write operations in a single burst

//     bool last = in.read_eos();
// prepare_burst_wide:
//     while (!last && (wc < WRITE_MAX_COUNT)) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         ap_uint<W> line = in.read();
//         last = in.read_eos();

//         out.write(line);
//         wc = wc + 1;

//         if (bc + 1 == BURST_LENGTH) {
//             // signal a complete burst
//             burst_size.write(BURST_LENGTH);
//             bc = 0;
//         } else {
//             bc = bc + 1;
//         }
//     }

//     // last burst or partial burst
//     if (bc != 0) {
//         burst_size.write(bc);
//     }
//     // no more writes
//     burst_size.write(0);

//     // propagate EOS
//     eos_signal.write(last);
// }

// // Possible variant: even if the stream is ended, complete the bust with empty
// // data. This could lead to waste less resources but increase latency for results
// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void prepare_burst(
//     STREAM_IN & in,
//     hls::stream< ap_uint<W> > & out,
//     hls::stream< ap_uint<8> > & burst_size,
//     hls::stream<bool> & eos_signal,
//     int out_size
// )
// {
//     using T = typename STREAM_IN::data_t;

//     HW_STATIC_ASSERT(W % sizeof(T) == 0,
//                      "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     constexpr int T_BITS = sizeof(T) * 8;           // item size in bits
//     constexpr int LINE_ITEMS = W / T_BITS;           // number of items in a write operation
//     const int WRITE_MAX_COUNT = out_size / (W / 8); // max number of write operations

//     int i = 0;  // index of tmp buffer
//     int bc = 0; // count the number of write operations in a single burst
//     int wc = 0; // count the total number of write operations

//     DataPack<T, LINE_ITEMS> line;
//     bool last = in.read_eos();

// prepare_burst:
//     while (!last && (wc < WRITE_MAX_COUNT)) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         T t = in.read();

//         if ((i + 1 == LINE_ITEMS) && (wc + 1 == WRITE_MAX_COUNT)) {
//             // last = true;
//         } else {
//             last = in.read_eos();
//         }

//         line[i] = t;

//         if (i + 1 == LINE_ITEMS) {
//             out.write(line);
//             i = 0;
//             wc = wc + 1;

//             // signal a complete burst
//             if (bc + 1 == BURST_LENGTH) {
//                 burst_size.write(BURST_LENGTH);
//                 bc = 0;
//             } else {
//                 bc = bc + 1;
//             }
//         } else {
//             i = i + 1;
//         }
//     }

//     // write remaining items
//     if (i != 0) {
//     remaining_items:
//         for (int j = 0; j < LINE_ITEMS; ++j) {
//         #pragma HLS unroll
//             if (j >= i) {
//                 line[j] = 0;
//             }
//         }
//         out.write(line);
//         bc = bc + 1;
//     }

//     // last burst or partial burst
//     if (bc != 0) {
//         burst_size.write(bc);
//     }
//     // no more writes
//     burst_size.write(0);

//     // propagate EOS
//     eos_signal.write(last);
// }

// template <int W, int BURST_LENGTH = 4096 / (W / 8)>
// void burst_write(
//     hls::stream< ap_uint<W> > & in,
//     hls::stream< ap_uint<8> > & burst_size,
//     hls::stream<bool> & eos_signal,
//     ap_uint<W> * out,
//     int * write_count,
//     int * eos
// )
// {
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");
//     int i = 0;
//     int bs = burst_size.read();
//     int wc = bs;

// burst_write:
//     while (bs) {
//     one_burst:
//         for (int j = 0; j < bs; j++) {
//         #pragma HLS pipeline II = 1
//         #pragma HLS LOOP_TRIPCOUNT min = 1 max = BURST_LENGTH
//             out[i * BURST_LENGTH + j] = in.read();
//         }
//         i++;
//         bs = burst_size.read();
//         wc += bs;
//     }

//     write_count[0] = wc;
//     eos[0] = (eos_signal.read() ? 1 : 0);
// }

// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void WStoWM(
//     STREAM_IN & in,
//     ap_uint<W> * out,
//     int out_size,
//     int * write_count,
//     int * eos
// )
// {
// #pragma HLS DATAFLOW
//     hls::stream< ap_uint<W> > internal_stream;
//     hls::stream< ap_uint<8> > burst_size;
//     hls::stream<bool> eos_signal;

//     constexpr int fifo_buf = 2 * BURST_LENGTH;

//     #pragma HLS STREAM variable = internal_stream depth = fifo_buf
//     #pragma HLS STREAM variable = burst_size depth = 2
//     #pragma HLS STREAM variable = eos_signal depth = 2

//     prepare_burst_wide(in, internal_stream, burst_size, eos_signal, out_size);
//     burst_write(internal_stream, burst_size, eos_signal, out, write_count, eos);
// }

// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void StoWM(
//     STREAM_IN & in,
//     ap_uint<W> * out,
//     int out_size,
//     int * write_count,
//     int * eos
// )
// {
// #pragma HLS DATAFLOW
//     hls::stream< ap_uint<W> > internal_stream;
//     hls::stream< ap_uint<8> > burst_size;
//     hls::stream<bool> eos_signal;

//     constexpr int fifo_buf = 2 * BURST_LENGTH;

//     #pragma HLS STREAM variable = internal_stream depth = fifo_buf
//     #pragma HLS STREAM variable = burst_size depth = 2
//     #pragma HLS STREAM variable = eos_signal depth = 2

//     prepare_burst(in, internal_stream, burst_size, eos_signal, out_size);
//     burst_write(internal_stream, burst_size, eos_signal, out, write_count, eos);
// }

// }

// #endif // __CONNECTORS_MEMORY_HPP__

#ifndef __CONNECTORS_MEMORY_HPP__
#define __CONNECTORS_MEMORY_HPP__


#include "ap_int.h"
#include "../common.hpp"
#include "../connectors/connectors.hpp"
#include "../datastructures/datastructures.hpp"
#include "../streams/streams.hpp"


namespace fx {

//******************************************************************************
//
// Memory to Stream
//
//******************************************************************************

template <int W, typename STREAM_OUT>
void WMtoWS(
    ap_uint<W> * in,
    int count,
    bool eos,
    STREAM_OUT & out
)
{
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");

WMtoWS:
    for (int i = 0; i < count; ++i) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        out.write(in[i]);
    }

    if (eos) {
        out.write_eos();
    }
}

template <int W, typename STREAM_OUT>
void WMtoS(
    ap_uint<W> * in,
    int count,
    bool eos,
    STREAM_OUT & out
)
{
    using T = typename STREAM_OUT::data_t;

    HW_STATIC_ASSERT(W % sizeof(T) == 0,
                     "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");

    constexpr int T_BITS = sizeof(T) * 8;   // item size in bits
    constexpr int READ_ITEMS = W / T_BITS;  // number of items in a read operation

WMtoS:
    for (int i = 0; i < count; ++i) {
    #pragma HLS PIPELINE II = READ_ITEMS
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        ap_uint<W> line = in[i];
        for (int j = 0; j < READ_ITEMS; ++j) {
        #pragma HLS PIPELINE II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
            ap_uint<T_BITS> item = line.range(T_BITS * (j + 1) - 1, T_BITS * j);
            out.write(TypeHandler<T>::from_ap(item));
        }
    }

    if (eos) {
        out.write_eos();
    }
}

// template <int W, typename STREAM_OUT>
// void WMtoSN_RR(
//     ap_uint<W> * in,
//     int count,
//     bool eos,
//     STREAM_OUT out[N_OUTS] // TODO: N_OUTS???
// )
// {
//     using T = typename STREAM_IN::data_t;

//     // HW_STATIC_ASSERT(N_OUTS == W / (sizeof(T) * 8),
//     //                  "N_OUTS must be equal to W / (sizeof(T) * 8)");
//     HW_STATIC_ASSERT(W % sizeof(T) == 0,
//                      "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     constexpr int N_OUTS = W / (sizeof(T) * 8);
//     constexpr int T_BITS = sizeof(T) * 8;   // item size in bits
//     constexpr int READ_ITEMS = N_OUTS;      // number of items in a read operation

// WMtoSN_RR:
//     for (int i = 0; i < count; ++i) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

//         ap_uint<W> line = in[i];
//         for (int j = 0; j < READ_ITEMS; ++j) {
//         #pragma HLS UNROLL
//             ap_uint<T_BITS> item = line.range(T_BITS * (j + 1) - 1, T_BITS * j);
//             out[j].write(TypeHandler<T>::from_ap(item));
//         }
//     }

//     if (eos) {
//         for (int i = 0; i < READ_ITEMS; ++i) {
//         #pragma HLS UNROLL
//             out[i].write_eos();
//         }
//     }
// }


//******************************************************************************
//
// Stream to Memory
//
//******************************************************************************

// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void prepare_burst_wide(
//     STREAM_IN & in,
//     hls::stream< ap_uint<W> > & out,
//     hls::stream< ap_uint<8> > & burst_size,
//     hls::stream<bool> & eos_signal,
//     int out_size
// )
// {
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     const int WRITE_MAX_COUNT = out_size / (W / 8); // max number of write operations

//     int wc = 0;   // count the total number of write operations
//     int bc = 0;   // count the number of write operations in a single burst

//     bool last = in.read_eos();
// prepare_burst_wide:
//     while (!last && (wc < WRITE_MAX_COUNT)) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         ap_uint<W> line = in.read();
//         last = in.read_eos();

//         out.write(line);
//         wc = wc + 1;

//         if (bc + 1 == BURST_LENGTH) {
//             // signal a complete burst
//             burst_size.write(BURST_LENGTH);
//             bc = 0;
//         } else {
//             bc = bc + 1;
//         }
//     }

//     // last burst or partial burst
//     if (bc != 0) {
//         burst_size.write(bc);
//     }
//     // no more writes
//     burst_size.write(0);

//     // propagate EOS
//     eos_signal.write(last);
// }

// Possible variant: even if the stream is ended, complete the bust with empty
// data. This could lead to waste less resources but increase latency for results
template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
void prepare_burst(
    STREAM_IN & in,
    hls::stream< ap_uint<W> > & out,
    hls::stream< ap_uint<8> > & burst_size,
    hls::stream< ap_uint<16> > & items_packed,
    hls::stream<bool> & eos_signal,
    int out_size
)
{
    using T = typename STREAM_IN::data_t;

    HW_STATIC_ASSERT(W % sizeof(T) == 0,
                     "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");

    constexpr int T_BITS = sizeof(T) * 8;           // item size in bits
    constexpr int TMP_ITEMS = W / T_BITS;           // number of items in a write operation
    const int WRITE_MAX_COUNT = out_size / (W / 8); // max number of write operations

    int i = 0;  // index of tmp buffer
    int wc = 0; // count the total number of write operations
    int bc = 0; // count the number of write operations in a single burst

    ap_uint<W> tmp;
    bool last = in.read_eos();

prepare_burst:
    while (!last && (wc < WRITE_MAX_COUNT)) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = in.read();

        if ((i + 1 == TMP_ITEMS) && (wc + 1 == WRITE_MAX_COUNT)) {
            // last = true;
        } else {
            last = in.read_eos();
        }

        tmp.range(T_BITS * (i + 1) - 1, T_BITS * i) = TypeHandler<T>::to_ap(t);

        if (i + 1 == TMP_ITEMS) {
            out.write(tmp);
            i = 0;
            wc = wc + 1;

            // signal a complete burst
            if (bc + 1 == BURST_LENGTH) {
                burst_size.write(BURST_LENGTH);
                items_pack.write(BURST_LENGTH * TMP_ITEMS);
                bc = 0;
            } else {
                bc = bc + 1;
            }
        } else {
            i = i + 1;
        }
    }

    // write remaining items
    if (i != 0) {
    remaining_items:
        for (int j = 0; j < TMP_ITEMS; ++j) {
        #pragma HLS unroll
            if (j >= i) {
                tmp.range(T_BITS * (j + 1) - 1, T_BITS * j) = 0;
            }
        }
        out.write(tmp);
        bc = bc + 1;
    }

    // last burst or partial burst
    if (bc != 0) {
        burst_size.write(bc);
        items_pack.write(bc * TMP_ITEMS + i);
    }
    // no more writes
    burst_size.write(0);
    items_pack.write(0);

    // propagate EOS
    eos_signal.write(last);
}

template <int W, int BURST_LENGTH = 4096 / (W / 8)>
void burst_write(
    hls::stream< ap_uint<W> > & in,
    hls::stream< ap_uint<8> > & burst_size,
    hls::stream< ap_uint<16> > & items_packed,
    hls::stream<bool> & eos_signal,
    ap_uint<W> * out,
    int * items_written,
    int * eos
)
{
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");
    int i = 0;
    int bs = burst_size.read();
    int iw = items_packed.read();

burst_write:
    while (bs) {
    one_burst:
        for (int j = 0; j < bs; j++) {
        #pragma HLS pipeline II = 1
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = BURST_LENGTH
            out[i * BURST_LENGTH + j] = in.read();
        }
        i++;
        bs = burst_size.read();
        iw += items_packed.read();
    }

    items_written[0] = iw;
    eos[0] = (eos_signal.read() ? 1 : 0);
}

// template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
// void WStoWM(
//     STREAM_IN & in,
//     ap_uint<W> * out,
//     int out_size,
//     int * write_count,
//     int * eos
// )
// {
// #pragma HLS DATAFLOW
//     hls::stream< ap_uint<W> > internal_stream;
//     hls::stream< ap_uint<8> > burst_size;
//     hls::stream<bool> eos_signal;

//     constexpr int fifo_buf = 2 * BURST_LENGTH;

//     #pragma HLS STREAM variable = internal_stream depth = fifo_buf
//     #pragma HLS STREAM variable = burst_size depth = 2
//     #pragma HLS STREAM variable = eos_signal depth = 2

//     prepare_burst_wide(in, internal_stream, burst_size, eos_signal, out_size);
//     burst_write(internal_stream, burst_size, eos_signal, out, write_count, eos);
// }

template <int W, int BURST_LENGTH = 4096 / (W / 8), typename STREAM_IN>
void StoWM(
    STREAM_IN & in,
    ap_uint<W> * out,
    int out_size,
    int * items_written,
    int * eos
)
{
#pragma HLS DATAFLOW
    hls::stream< ap_uint<W> > internal_stream;
    hls::stream< ap_uint<8> > burst_size;
    hls::stream< ap_uint<16> > items_packed;
    hls::stream<bool> eos_signal;

    constexpr int fifo_buf = 2 * BURST_LENGTH;

    #pragma HLS STREAM variable = internal_stream depth = fifo_buf
    #pragma HLS STREAM variable = burst_size depth = 2
    #pragma HLS STREAM variable = items_packed depth = 2
    #pragma HLS STREAM variable = eos_signal depth = 2

    prepare_burst(in, internal_stream, burst_size, items_packed, eos_signal, out_size);
    burst_write(internal_stream, burst_size, items_packed, eos_signal, out, items_written, eos);
}


// template <typename T, int DEPTH_IN, int W, int BURST_LENGTH = 4096 / (W / 8)>
// void prepare_burst(
//     fx::axis_stream<T, DEPTH_IN> & in,
//     hls::stream< ap_uint<W> > & out,
//     hls::stream< ap_uint<8> > & burst_size,
//     hls::stream<bool> & eos_signal,
//     int out_size
// )
// {
//     HW_STATIC_ASSERT(W % sizeof(T) == 0,
//                      "AXI port width W is not multiple of stream element width (sizeof(T) * 8).");
//     HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
//                      "AXI port width W must be power of 2 and between 8 to 512.");

//     constexpr int T_BITS = sizeof(T) * 8;           // item size in bits
//     constexpr int TMP_ITEMS = W / T_BITS;           // number of items in a write operation
//     const int WRITE_MAX_COUNT = out_size / (W / 8); // max number of write operations

//     int i = 0;  // index of tmp buffer
//     int wc = 0; // count the total number of write operations
//     int bc = 0; // count the number of write operations in a single burst

//     ap_uint<W> tmp;
//     bool last = false;

// prepare_burst:
//     while (!last && (wc < WRITE_MAX_COUNT)) {
//     #pragma HLS PIPELINE II = 1
//     #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
//         T t = in.read(last);
//         tmp.range(T_BITS * (i + 1) - 1, T_BITS * i) = TypeHandler<T>::to_ap(t);

//         if (i + 1 == TMP_ITEMS) {
//             out.write(tmp);
//             i = 0;
//             wc = wc + 1;

//             // signal a complete burst
//             if (bc + 1 == BURST_LENGTH) {
//                 burst_size.write(BURST_LENGTH);
//                 bc = 0;
//             } else {
//                 bc = bc + 1;
//             }
//         } else {
//             i = i + 1;
//         }
//     }

//     // write remaining items
//     if (i != 0) {
//     remaining_items:
//         for (int j = 0; j < TMP_ITEMS; ++j) {
//         #pragma HLS unroll
//             if (j >= i) {
//                 tmp.range(T_BITS * (j + 1) - 1, T_BITS * j) = 0;
//             }
//         }
//         out.write(tmp);
//         bc = bc + 1;
//     }

//     // last burst or partial burst
//     if (bc != 0) {
//         burst_size.write(bc);
//     }
//     // no more writes
//     burst_size.write(0);

//     // propagate EOS
//     eos_signal.write(last);
// }

// template <typename T, int DEPTH_IN, int W, int BURST_LENGTH = 4096 / (W / 8)>
// void AStoWM(
//     fx::axis_stream<T, DEPTH_IN> & in,
//     ap_uint<W> * out,
//     int out_size,
//     int * write_count,
//     int * eos
// )
// {
// #pragma HLS DATAFLOW
//     hls::stream< ap_uint<W> > internal_stream;
//     hls::stream< ap_uint<8> > burst_size;
//     hls::stream<bool> eos_signal;

//     constexpr int fifo_buf = 2 * BURST_LENGTH;

//     #pragma HLS STREAM variable = internal_stream depth = fifo_buf
//     #pragma HLS STREAM variable = burst_size depth = 2
//     #pragma HLS STREAM variable = eos_signal depth = 2

//     prepare_burst(in, internal_stream, burst_size, eos_signal, out_size);
//     burst_write(internal_stream, burst_size, eos_signal, out, write_count, eos);
// }

}

#endif // __CONNECTORS_MEMORY_HPP__