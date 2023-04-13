#include "kernel.hpp"

void process_1(
    in_stream_t & in,
    out_stream_t & out
)
{
    bool last = in.read_eos();
PROCESS_1:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        data_t d = in.read();
        last = in.read_eos();
        d.y[1] = d.y[1] * 2;
        out.write(d);
    }

    out.write_eos();
}

void process_2(
    in_stream_t & in,
    out_stream_t & out
)
{
    bool last = in.read_eos();
PROCESS_2:
    while (!last) {
    #pragma HLS PIPELINE II = 2
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        data_t d = in.read();
        last = in.read_eos();
        d.y[1] = d.y[1] * 2;
        out.write(d);
    }

    out.write_eos();
}

void process_3(
    in_stream_t & in,
    out_stream_t & out
)
{
    bool last = in.read_eos();
PROCESS_3:
    while (!last) {
    #pragma HLS PIPELINE II = 3
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        data_t d = in.read();
        last = in.read_eos();
        d.y[1] = d.y[1] * 2;
        out.write(d);
    }

    out.write_eos();
}


void process_4(
    in_stream_t & in,
    out_stream_t & out
)
{
    bool last = in.read_eos();
PROCESS_4:
    while (!last) {
    #pragma HLS PIPELINE II = 4
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        data_t d = in.read();
        last = in.read_eos();
        d.y[1] = d.y[1] * 2;
        out.write(d);
    }

    out.write_eos();
}



/**
    W = real_work
    w = fake_work
    s = sleep
    B = blocked
                                                    | 1                                     | 2                                     | 3
    CLK     | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | 1 |
    W_0     | w | s | s | s | s | s | B | B | W | s | s | s | s | s | B | B | W | s | s | s | s | s | B | B | W | s | s | s | s | s | B | B |
    W_1     | s | s | s | s | B | W | w | s | s | s | s | B | B | W | w | s | s | s | s | B | B | W | w | s | s | s | s | B | B | W | w | s |
    W_2     | B | B | W | w | w | s | s | s | B | B | W | w | w | s | s | s | B | B | W | w | w | s | s | s | B | B | W | w | w | s | s | s |
    W_3     | s | s | B | W | w | w | w | s | s | B | B | W | w | w | w | s | s | B | B | W | w | w | w | s | s | B | B | W | w | w | w | s |
*/

void compute(
    in_stream_t in[PAR],
    out_stream_t out[PAR]
)
{
#pragma HLS DATAFLOW
    process_1(in[0], out[0]);
    process_2(in[1], out[1]);
    process_3(in[2], out[2]);
    process_4(in[3], out[3]);
}

void test(
    stream_t & in,
    stream_t & out
)
{
    in_stream_t in_streams[PAR];
    out_stream_t out_streams[PAR];

#pragma HLS DATAFLOW
    fx::StoSN_LB<PAR>(in, in_streams);
    compute(in_streams, out_streams);
    fx::SNtoS_LB<PAR>(out_streams, out);
}
