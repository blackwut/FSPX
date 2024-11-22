#ifndef __CONNECTORS_MEMORY_HPP__
#define __CONNECTORS_MEMORY_HPP__

#include "ap_int.h"
#include "../common.hpp"
#include "../connectors/connectors.hpp"
#include "../datastructures/typehandler.hpp"
#include "../streams/streams.hpp"


namespace fx {

//******************************************************************************
//
// Memory to Stream
//
//******************************************************************************

template <
    unsigned int W,
    typename stream_out_t
>
void WMtoS (
    ap_uint<W> * in,
    unsigned int count,
    bool eos,
    stream_out_t & out
)
{
    using data_t = typename stream_out_t::data_t;

    HW_STATIC_ASSERT(W % sizeof(data_t) == 0,
                     "AXI port width W is not multiple of stream element width (sizeof(data_t) * 8).");
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");

    constexpr int SIZE_IN_BITS = sizeof(data_t) * 8;  // item size in bits
    constexpr int READ_ITEMS = W / SIZE_IN_BITS;      // number of items in a read operation

    WMtoS:
    for (unsigned int i = 0; i < count; ++i) {
        #pragma HLS PIPELINE II = READ_ITEMS
        #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024

        ap_uint<W> line = in[i];
        for (unsigned int j = 0; j < READ_ITEMS; ++j) {
            #pragma HLS PIPELINE II = 1
            #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
            
            const ap_uint<SIZE_IN_BITS> item = line.range(SIZE_IN_BITS * (j + 1) - 1, SIZE_IN_BITS * j);
            out.write(TypeHandler<data_t>::from_ap(item));
        }
    }

    if (eos) {
        out.write_eos();
    }
}

template <
    unsigned int W,
    unsigned int BURST_LENGTH = 4096 / (W / 8),
    typename stream_in_t
>
void prepare_burst (
    stream_in_t & in,
    hls::stream< ap_uint<W> > & out,
    hls::stream< ap_uint<8> > & burst_size,
    hls::stream< ap_uint<16> > & items_packed,
    hls::stream<bool> & eos_signal,
    unsigned int out_size
)
{
    using data_t = typename stream_in_t::data_t;

    HW_STATIC_ASSERT(W % sizeof(data_t) == 0,
                     "AXI port width W is not multiple of stream element width (sizeof(data_t) * 8).");
    HW_STATIC_ASSERT((W >= 8) && (W <= 512) && IS_POW2(W),
                     "AXI port width W must be power of 2 and between 8 to 512.");

    constexpr unsigned int SIZE_IN_BITS = sizeof(data_t) * 8;         // item size in bits
    constexpr unsigned int TMP_ITEMS = W / SIZE_IN_BITS;              // number of items in a write operation
    const unsigned int WRITE_MAX_COUNT = out_size / (W / 8);    // max number of write operations

    unsigned int i = 0;  // index of tmp buffer
    unsigned int wc = 0; // count the total number of write operations
    unsigned int bc = 0; // count the number of write operations in a single burst

    ap_uint<W> tmp;
    bool last = in.read_eos();

    PREPARE_BURST:
    while (!last && (wc < WRITE_MAX_COUNT)) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        data_t t = in.read();

        if ((i + 1 == TMP_ITEMS) && (wc + 1 == WRITE_MAX_COUNT)) {
            // last = true;
        } else {
            last = in.read_eos();
        }

        tmp.range(SIZE_IN_BITS * (i + 1) - 1, SIZE_IN_BITS * i) = TypeHandler<data_t>::to_ap(t);

        if (i + 1 == TMP_ITEMS) {
            out.write(tmp);
            i = 0;
            wc = wc + 1;

            // signal a complete burst
            if (bc + 1 == BURST_LENGTH) {
                burst_size.write(BURST_LENGTH);
                items_packed.write(BURST_LENGTH * TMP_ITEMS);
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
        REMAINING_ITEMS:
        for (int j = 0; j < TMP_ITEMS; ++j) {
            #pragma HLS unroll
            if (j >= i) {
                tmp.range(SIZE_IN_BITS * (j + 1) - 1, SIZE_IN_BITS * j) = 0;
            }
        }
        out.write(tmp);
        bc = bc + 1;
    }

    // last burst or partial burst
    if (bc != 0) {
        burst_size.write(bc);
        items_packed.write(bc * TMP_ITEMS + i);
    }
    // no more writes
    burst_size.write(0);
    items_packed.write(0);

    // propagate EOS
    eos_signal.write(last);
}

template <
    unsigned int W,
    unsigned int BURST_LENGTH = 4096 / (W / 8)
>
void burst_write (
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
    unsigned int i = 0;
    unsigned int bs = burst_size.read();
    unsigned int iw = items_packed.read();

    BURST_WRITE:
    while (bs) {
        BURST:
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

template <
    unsigned int W,
    unsigned int BURST_LENGTH = 4096 / (W / 8),
    typename stream_in_t>
void StoWM (
    stream_in_t & in,
    ap_uint<W> * out,
    unsigned int out_size,
    int * items_written,
    int * eos
)
{
#pragma HLS DATAFLOW
    hls::stream< ap_uint<W> > internal_stream;
    hls::stream< ap_uint<8> > burst_size;
    hls::stream< ap_uint<16> > items_packed;
    hls::stream<bool> eos_signal;

    constexpr unsigned int FIFO_BUFFER = 2 * BURST_LENGTH;

    #pragma HLS STREAM variable = internal_stream depth = FIFO_BUFFER
    #pragma HLS STREAM variable = burst_size      depth = 2
    #pragma HLS STREAM variable = items_packed    depth = 2
    #pragma HLS STREAM variable = eos_signal      depth = 2

    prepare_burst(in, internal_stream, burst_size, items_packed, eos_signal, out_size);
    burst_write(internal_stream, burst_size, items_packed, eos_signal, out, items_written, eos);
}

}

#endif // __CONNECTORS_MEMORY_HPP__