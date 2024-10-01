#include "kernel.hpp"
// #include "../../include/datastructures/bucket.hpp" // needed for result_t

// #define PRODUCER_CONSUMER_TEST

#include <stdint.h>

#if defined(MULTISTREAM)

#define N 4


template <typename STREAM_IN, typename STREAM_OUT>
void duplicate_data(STREAM_IN & in, STREAM_OUT out[N])
{
    bool last = in.read_eos();
    DUPLICATE_WHILE:
    while (!last) {
    #pragma HLS PIPELINE II=1

        data_t data = in.read();
        last = in.read_eos();

        DUPLICATE_FOR:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            out[i].write(data);
        }
    }

    DUPLICATE_EOS:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        out[i].write_eos();
    }
}

template <typename STREAM_IN, typename STREAM_OUT>
void multistream(STREAM_IN in[N], STREAM_OUT & out)
{

    int id = 100;
    ap_uint<N> lasts = 0;
    const ap_uint<N> ends = ~lasts;

    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lasts[i] = in[i].read_eos();
    }

    MULTISTREAM_WHILE:
    while (lasts != ends) {
    #pragma HLS PIPELINE II=1
        id++;

        int cache[N];
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            cache[i] = -1;
        }

        MULTISTREAM_READ:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            if (!lasts[i]) {
                data_t d = in[i].read();
                lasts[i] = in[i].read_eos();

                cache[i] = d.key;
            }
        }

        int sum = 0;
        MULTISTREAM_SUM:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            if (cache[i] != -1) {
                sum += cache[i];
            }
        }

        data_t out_data;
        out_data.key = id;
        out_data.value = 0;
        out_data.aggregate = 0;
        out_data.timestamp = sum;

        out.write(out_data);
    }
    out.write_eos();
}


void test(in_stream_t & in, out_stream_t & out)
{
    fx::stream<data_t, N * N> data[N];
    // #pragma HLS array_partition variable = data complete 

#define HLS DATAFLOW

    duplicate_data(in, data);
    multistream(data, out);
}

#endif

#if defined(COMPACTOR)

#include "hls_streamofblocks.h"

#define N 4

using val_t = int;
using block_t = val_t[N];
using stream_blocks_t = hls::stream_of_blocks<block_t>;


void generate_data(stream_blocks_t & data)
{
    hls::write_lock<block_t> lock(data);

    static int x = 0;
    GENERATE_DATA:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        lock[i] = (i == x || i == x + 1 ? i : -1);
    }

    if (x == N - 1) {
        x = 0;
    } else {
        x++;
    }
}

template <typename CMP>
void compact_pe(int idx, stream_blocks_t & in, stream_blocks_t & out, CMP && cmp)
{

    hls::read_lock<block_t> lock_in(in);
    hls::write_lock<block_t> lock_out(out);

    std::cout << "compact_pe<" << idx << ">: ";
    for (int i = 0; i < N; ++i) {
        std::cout << lock_in[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "move: ";
    ap_uint<N> move = 0;
    MOVE:
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        move[i] = cmp(lock_in[i]);
        std::cout << move[i] << " ";
    }
    std::cout << std::endl;

    COMPACT:
    for (int i = 0; i < N - 1; ++i) {
    #pragma HLS UNROLL
        if (move.range(i, 0) == 0) {
            lock_out[i] = lock_in[i];
        } else {
            lock_out[i] = lock_in[i + 1];
        }
    }

    lock_out[N - 1] = -1;
}

template <unsigned int SIZE, typename STREAM_OUT>
struct buffer_shipper
{
    using WIN_T = unsigned int;

    WIN_T tail;
    val_t shift_reg[SIZE];

    buffer_shipper()
    : tail(0)
    {
        #pragma HLS ARRAY_PARTITION variable = shift_reg complete

        for (int i = 0; i < SIZE; ++i) {
        #pragma HLS UNROLL
            shift_reg[i] = -1;
        }
    }

    void print()
    {
        std::cout << "shift_reg: ";
        PRINT_SHIFT:
        for (int i = 0; i < SIZE; ++i) {
            std::cout << shift_reg[i] << " ";
        }
        std::cout << std::endl;
    }

    // insert len elements of the circular buffer (cbuffer) from the start index in shift register
    void insert(stream_blocks_t & in, const WIN_T len, STREAM_OUT & shipper)
    {
        hls::read_lock<block_t> cbuffer(in);

        std::cout << "buffer_shipper in: ";
        for (int i = 0; i < N; ++i) {
            std::cout << cbuffer[i] << " ";
        }
        std::cout << std::endl;

        const WIN_T old_tail = tail;
        if (tail + len > 0) {
            tail = tail + len - 1;
        } else {
            tail = 0;
        }


        COPY_SHIPPER:
        for (WIN_T i = 0; i < SIZE; ++i) {
        #pragma HLS UNROLL
            if (i >= old_tail) {
                shift_reg[i] = cbuffer[i - old_tail];
            }
        }

        std::cout << "old_tail: " << old_tail << std::endl;
        std::cout << "tail: " << tail << std::endl;

        print();

        const val_t state = shift_reg[0];
        SHIFT_SHIPPER:
        for (WIN_T i = 0; i < SIZE - 1; ++i) {
        #pragma HLS UNROLL
            shift_reg[i] = shift_reg[i + 1];
        }
        shift_reg[SIZE - 1] = -1;

        std::cout << "state: " << state << std::endl;
        if (state != -1) {
            shipper.write(state);
        }

        print();
    }
};

// void output_data(stream_blocks_t & data, fx::stream<int, 2> & out)
// {
//     hls::read_lock<block_t> lock(data);
//     OUTPUT_DATA:
//     for (int i = 0; i < N; ++i) {
//         out.write(lock[i]);
//     }
//     out.write_eos();
// }

void compact_and_send(buffer_shipper<N * N, fx::stream<int, 2>> & bshipper, fx::stream<int, 2> & out)
{
    #pragma HLS DATAFLOW

    stream_blocks_t data[N];
    #pragma HLS array_partition variable = data complete

    generate_data(data[0]);    

    REPLICATE_PE:
    for (int i = 0; i < N - 1; ++i) {
        #pragma HLS UNROLL
        compact_pe(i, data[i], data[i + 1], [](int x) { return x == -1; });
    }

    bshipper.insert(data[N - 1], 2, out);
}


void test(fx::stream<int, 2> & in, fx::stream<int, 2> & out)
{
    buffer_shipper<N * N, fx::stream<int, 2>> bshipper;

    bool last = in.read_eos();
    while (!last) {
    #pragma HLS PIPELINE II=1
        int data = in.read();
        last = in.read_eos();

        compact_and_send(bshipper, out);
    }

    out.write_eos();
}

#endif


#if defined(INSERT_SORT)


// #include <iomanip>

// template <typename T>
// void print_array(std::string name, T * array, int size)
// {
//     std::cout << name << "(";
//     for (int i = 0; i < size; ++i) {
//         std::cout << std::setw(5) << array[i] << " ";
//     }
//     std::cout << ")" << std::endl;
// }

// template <>
// void print_array<bool>(std::string name, bool * array, int size)
// {
//     std::cout << name << "(";
//     for (int i = 0; i < size; ++i) {
//         std::cout << std::setw(5) << (array[i] ? "T" : "F") << " ";
//     }
//     std::cout << ")" << std::endl;
// }

template <typename Data_Type, typename Key_Type, int max_sort_number>
void insert_sort_top(
    hls::stream<Data_Type>& din_strm,
    hls::stream<Key_Type>& kin_strm,
    hls::stream<bool>& strm_in_end,
    hls::stream<Data_Type>& dout_strm,
    hls::stream<Key_Type>& kout_strm,
    hls::stream<bool>& strm_out_end,
    bool sign
) {
    
    bool end;
    Key_Type in_temp;
    Key_Type out_temp;
    Data_Type in_dtemp;
    Data_Type out_dtemp;
    bool array_full = 0;

    static Key_Type array_temp[max_sort_number];
    static Data_Type array_dtemp[max_sort_number];
    bool comparative_sign[max_sort_number];

#pragma HLS ARRAY_PARTITION variable = array_temp complete
#pragma HLS ARRAY_PARTITION variable = array_dtemp complete
#pragma HLS ARRAY_PARTITION variable = comparative_sign complete

    comparative_sign[0] = 0;

    uint16_t inserting_id = 1;
    uint16_t residual_count = 1;
    uint16_t begin = 0;
    end = strm_in_end.read();

insert_loop:
    while (!end || residual_count) {
    #pragma HLS PIPELINE ii = 1

        // read input strm
        if (!end) {
            in_temp = kin_strm.read();
            in_dtemp = din_strm.read();
            end = strm_in_end.read();
            if (begin < max_sort_number) {
                residual_count++;
                begin++;
            }
        } else {
        }

    // initialize sign
    initial_sign_loop:
        for (int i = 1; i < max_sort_number; i++) {
    #pragma HLS UNROLL
            if (i < inserting_id) {
                if (array_temp[i - 1] < in_temp) { // array_temp[i - 1] < max_wid - N + 1
                    comparative_sign[i] = sign;
                } else {
                    comparative_sign[i] = !sign;
                }
            } else {
                comparative_sign[i] = 1;
            }
        }

        // print_array<bool>("sign", comparative_sign, max_sort_number);

        // manage the last element
        out_temp = array_temp[begin - 1];
        out_dtemp = array_dtemp[begin - 1];
        if (comparative_sign[max_sort_number - 1] == 0) {
            array_temp[max_sort_number - 1] = in_temp;
            array_dtemp[max_sort_number - 1] = in_dtemp;
        } else {
        }

    // right shift && insert for intermediate elements
    right_shift_insert_loop:
        for (int i = max_sort_number - 2; i >= 0; i--) {
#pragma HLS UNROLL
            if (comparative_sign[i] == 0 && comparative_sign[i + 1] == 0) {
            } else if (comparative_sign[i] == 0 && comparative_sign[i + 1] == 1) {
                array_temp[i + 1] = array_temp[i];
                array_dtemp[i + 1] = array_dtemp[i];
                array_temp[i] = in_temp;
                array_dtemp[i] = in_dtemp;
            } else if (comparative_sign[i] == 1 && comparative_sign[i + 1] == 1) {
                array_temp[i + 1] = array_temp[i];
                array_dtemp[i + 1] = array_dtemp[i];
            } else {
            }
        }

        // print_array<Key_Type>("keys", array_temp, max_sort_number);
        // print_array<Data_Type>("data", array_dtemp, max_sort_number);

        // if (residual_count % 4 == 0) {
        //     LEFT_SHIFT:
        //     for (int i = 0; i < max_sort_number - 1; i++) {
        //         array_temp[i] = array_temp[i + 1];
        //         array_dtemp[i] = array_dtemp[i + 1];
        //     }
        // }

        // write output strm
        if (array_full) {
            kout_strm.write(out_temp);
            dout_strm.write(out_dtemp);
            strm_out_end.write(0);
        } else {
        }

        // update loop parameters
        if (end) {
            inserting_id = 1;
            residual_count--;
            array_full = 1;
        } else {
            if (inserting_id == max_sort_number) {
                inserting_id = 1;
                array_full = 1;
            } else {
                inserting_id++;
            }
        }
    }
    strm_out_end.write(1);
}

template <typename Data_Type, typename Key_Type>
void source(
    in_stream_t & in,
    hls::stream<Data_Type> & din_strm,
    hls::stream<Key_Type> & kin_strm,
    hls::stream<bool> & strm_in_end
)
{
    bool last = in.read_eos();
    SOURCE:
    while (!last) {
    #pragma HLS PIPELINE II=1
        data_t data = in.read();
        last = in.read_eos();

        din_strm.write(data);
        kin_strm.write(data.key);
        strm_in_end.write(false);
    }
    strm_in_end.write(true);
}


template <typename Data_Type, typename Key_Type>
void sink(
    out_stream_t & out,
    hls::stream<Data_Type> & dout_strm,
    hls::stream<Key_Type> & kout_strm,
    hls::stream<bool> & strm_out_end
)
{
    bool last = strm_out_end.read();
    SINK:
    while (!last) {
    #pragma HLS PIPELINE II=1
        Data_Type data = dout_strm.read();
        Key_Type key = kout_strm.read();
        last = strm_out_end.read();
        
        data_t out_data = data;
        out.write(out_data);
    }
    out.write_eos();
}

void test(in_stream_t & in, out_stream_t & out)
{
    static constexpr unsigned int stream_depth = 4;
    static constexpr unsigned int max_sort_number = 4;
    static constexpr bool sign = 1;

    using Data_Type = data_t;
    using Key_Type = unsigned int;

    #pragma HLS DATAFLOW

    hls::stream<Data_Type> din_strm;
    hls::stream<Key_Type> kin_strm;
    hls::stream<bool> strm_in_end;
    hls::stream<Data_Type> dout_strm;
    hls::stream<Key_Type> kout_strm;
    hls::stream<bool> strm_out_end;

    #pragma HLS STREAM variable=din_strm depth=stream_depth
    #pragma HLS STREAM variable=kin_strm depth=stream_depth
    #pragma HLS STREAM variable=strm_in_end depth=stream_depth
    #pragma HLS STREAM variable=dout_strm depth=stream_depth
    #pragma HLS STREAM variable=kout_strm depth=stream_depth
    #pragma HLS STREAM variable=strm_out_end depth=stream_depth



    source<Data_Type, Key_Type>(in, din_strm, kin_strm, strm_in_end);
    insert_sort_top<Data_Type, Key_Type, max_sort_number>(
        din_strm,
        kin_strm,
        strm_in_end,
        dout_strm,
        kout_strm,
        strm_out_end,
        sign
    );
    sink<Data_Type, Key_Type>(out, dout_strm, kout_strm, strm_out_end);
}

#endif

#if defined(LATE_TIME_TUMBLING_WINDOW_OPERATOR)

template <typename OP>
struct Drainer
{

    void operator()(const fx::time_result_t<OP> in, data_t & out) {
    #pragma HLS INLINE

        std::cout << "Drainer: in.wid = " << in.wid << " in.value = " << in.value << " in.timestamp = " << in.timestamp << std::endl;

        out.key = 0;
        out.value = 0;
        out.aggregate = in.value;
        out.timestamp = in.timestamp;
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using OP = fx::Count<float>;
    fx::stream<fx::time_result_t<OP>, 16> result_stream("result_stream");

    #pragma HLS DATAFLOW

    fx::TimeTumblingWindowOperator<OP, WINDOW_SIZE, WINDOW_LATENESS>(
        in, result_stream
    );

    fx::Map<Drainer<OP>>(
        result_stream, out
    );
}
#endif

#if defined(KEYED_LATE_TIME_TUMBLING_WINDOW_OPERATOR)

template <typename OP, typename KEY_T>
struct Drainer
{
    void operator()(const fx::keyed_time_result_t<OP, KEY_T> in, data_t & out) {
    #pragma HLS INLINE

        // std::cout << "Drainer: in.key = " << in.key << " in.wid = " << in.wid << " in.value = " << in.value << " in.timestamp = " << in.timestamp << std::endl;
        // std::cout << in << std::endl;

        out.key = in.key;
        out.value = 0;
        out.aggregate = in.value;
        out.timestamp = in.timestamp;
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using KEY_T = unsigned int;
    using OP = fx::Count<float>;
    fx::stream<fx::keyed_time_result_t<OP, KEY_T>, 16> result_stream("result_stream");

    #pragma HLS DATAFLOW

    fx::KeyedTimeTumblingWindowOperator<OP, MAX_KEYS, WINDOW_SIZE, WINDOW_LATENESS>(
        in, result_stream, [](const data_t & d) { return d.key; }
    );

    fx::Map<Drainer<OP, KEY_T>>(
        result_stream, out
    );
}
#endif

#if defined(LATE_TIME_TUMBLING_WINDOW)
template <typename OP>
struct LateTimeTumblingWindowFunctor
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    fx::Bucket::LateTimeTumblingWindow<OP, WINDOW_SIZE, WINDOW_LATENESS> window;

    template <typename T>
    void operator()(const data_t in, fx::FlatMapShipper<T> & shipper) {
    #pragma HLS INLINE
        const IN_T value = in.value;
        const unsigned int timestamp = in.timestamp;
        window.update(value, timestamp, shipper.ostrm);
    }
};

template <typename OP>
struct Drainer
{
    using result_t = typename fx::Bucket::late_time_bucket_t<OP, WINDOW_SIZE, WINDOW_LATENESS>::result_t;

    void operator()(const result_t in, data_t & out) {
    #pragma HLS INLINE

        std::cout << "Drainer: in.value = " << in.value << " in.timestamp = " << in.timestamp << std::endl;

        out.key = 0;
        out.value = 0;
        out.aggregate = in.value;
        out.timestamp = in.timestamp;
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using OP = fx::Count<float>;
    static constexpr unsigned int L = OP::LATENCY;
    using result_t = typename fx::Bucket::late_time_bucket_t<OP, WINDOW_SIZE, WINDOW_LATENESS>::result_t;


    fx::stream<result_t, 4> result_stream;

    #pragma HLS DATAFLOW

    fx::FlatMap<LateTimeTumblingWindowFunctor<OP>, L>(
        in, result_stream
    );

    fx::Map<Drainer<OP>>(
        result_stream, out
    );
}
#endif

#if defined(KEYED_TIME_TUMBLING_WINDOW)
template <typename OP>
struct KeyedTimeTumblingWindowFunctor
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    fx::Bucket::KeyedTimeTumblingWindow<OP, MAX_KEYS, WINDOW_SIZE> window;

    template <typename T>
    void operator()(const data_t in, fx::FlatMapShipper<T> & shipper) {
    #pragma HLS INLINE
        const unsigned int key = in.key;
        const IN_T value = in.value;
        const unsigned int timestamp = in.timestamp;

        OUT_T agg = OP::lower(OP::identity());
        bool valid = window.update(key, value, timestamp, agg);

        if (valid) {
            data_t out;
            out.key = key;
            out.value = value;
            out.aggregate = agg;
            out.timestamp = timestamp;
            shipper.send(out);
        }
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using OP = fx::Count<float>;
    static constexpr unsigned int L = OP::LATENCY;

    #pragma HLS DATAFLOW

    fx::FlatMap<KeyedTimeTumblingWindowFunctor<OP>, L>(
        in, out
    );
}
#endif

#if defined(PRODUCER_CONSUMER_TEST)

#define SIZE 6
void producer(int idx, int from, int to, int out[SIZE])
{
    unsigned int size = (from <= to) ? (to - from) : (SIZE - from + to);
    if (size == 0) {
        for (int i = 0; i < SIZE; ++i) {
        #pragma HLS UNROLL
            out[i] = -1;
        }
    } else {

        for (int i = 0; i < SIZE; ++i) {
        #pragma HLS UNROLL
            // out[i] = (i >= from && i < to) ? idx : -1;
            if (from <= to) {
                out[i] = (i >= from && i < to) ? idx : -1;
            } else {
                out[i] = (i >= from || i < to) ? idx : -1;
            }
        }
    }

    std::cout << "out[] = {";
    for (int i = 0; i < SIZE; ++i) {
        std::cout << out[i] << " ";
    }
    std::cout << "}" << std::endl;
}

void consumer(int in[SIZE], int from, int to, int & out)
{
    static int buffer[SIZE + 1];// = {-2, -2, -2, -2, -2, -2};
    static int last = 0;

    const bool wraparound = (from > to);
    const unsigned int left_size = wraparound ? to : (to - from);
    const unsigned int right_size = wraparound ? (SIZE - from) : 0;
    const unsigned int in_size = left_size + right_size;

    assert(in_size < SIZE);

    int old_last = last;
    last = (last - 1) + in_size;

    if (in_size > 0) {
        for (int i = 0; i < SIZE; ++i) {
        #pragma HLS UNROLL
            if (i >= old_last && i <= last) {
                const unsigned int base_idx = i - old_last;
                if (i < old_last + left_size) {
                    buffer[i] = in[from + base_idx];
                } else {
                    buffer[i] = in[from + base_idx + left_size];
                }
            }
        }

        // for (int i = 0; i < SIZE + 1; ++i) {
        // #pragma HLS UNROLL
        //     if (i >= old_last && i <= last) {
        //         buffer[i] = in[from + i - old_last]; // from + i - old_last cannot reach SIZE + 1
        //     }
        // }
    }

    // print buffer
    for (int i = 0; i < SIZE + 1; ++i) {
        std::cout << buffer[i] << " ";
    }
    std::cout << std::endl;

    out = buffer[0];
    SHIFT_BUFFER:
    for (int i = 0; i < SIZE; ++i) {
    #pragma HLS PIPELINE II=1
        buffer[i] = buffer[i + 1];
    }
    buffer[SIZE] = 0;
}

void _test_prod_cons(int i, int from, int to, int buff[SIZE])
{
    #pragma HLS DATAFLOW
    producer(i, from, to, buff);
    int out = -1;
    consumer(buff, from, to, out);
    std::cout << out << std::endl;
}

void test_prod_cons()
{
    static int buff[SIZE];
    int capacity = SIZE;

    // initialize buffer
    for (int i = 0; i < SIZE; ++i) {
    #pragma HLS UNROLL
        buff[i] = -1;
    }

    int total = 0;

    for (int i = 0; i < 8; ++i) {
    #pragma HLS PIPELINE II=1
        int size = ((i + 3) * 7) % 5;
        if (size > capacity) {
            size = capacity;
            capacity = 0;
        } else {
            capacity -= size;
        }

        int from = total % SIZE;
        int to = (from + size) % SIZE;
        total += size;

        std::cout << "idx: " << i << " from: " << from << " to: " << to << " size: " << size << " capacity: " << capacity << std::endl;
        _test_prod_cons(i, from, to, buff);
        capacity++;
    }

}

void test(in_stream_t & in, out_stream_t & out)
{
    TEST_MULTI_WRITE:
    bool last = in.read_eos();
    while (!last) {
    #pragma HLS PIPELINE II=1
        data_t data = in.read();
        last = in.read_eos();
        data_t out_data;
        out_data.key = data.key;
        out_data.value = data.value;
        out_data.aggregate = data.aggregate;
        out_data.timestamp = data.timestamp;
        out.write(out_data);
    }
    out.write_eos();

    test_prod_cons();
}
#endif

#if defined(TIME_TUMBLING_WINDOW)
template <typename OP>
struct TimeTumblingWindowFunctor
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    fx::Bucket::TimeTumblingWindow<OP, WINDOW_SIZE, OP::LATENCY> window;

    template <typename T>
    void operator()(const data_t in, fx::FlatMapShipper<T> & shipper) {
    #pragma HLS INLINE
        const unsigned int key = in.key;
        const IN_T value = in.value;
        const unsigned int timestamp = in.timestamp;

        OUT_T agg = OP::lower(OP::identity());
        bool valid = window.update(value, timestamp, agg);

        if (valid) {
            data_t out;
            out.key = key;
            out.value = value;
            out.aggregate = agg;
            out.timestamp = timestamp;
            shipper.send(out);
        }
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using OP = fx::Count<float>;
    static constexpr unsigned int L = OP::LATENCY;

    #pragma HLS DATAFLOW

    fx::FlatMap<TimeTumblingWindowFunctor<OP>, 1>(
        in, out
    );
}
#endif

#if defined(KEYED_COUNT_TUMBLING_WINDOW)
template <typename OP>
struct KeyedCountTumblingWindowFunctor
{
    static constexpr unsigned int L = OP::LATENCY;
    using IN_T = typename OP::IN_T;
    using AGG_T = typename OP::AGG_T;
    using OUT_T = typename OP::OUT_T;

    fx::Bucket::KeyedCountTumblingWindow<OP, MAX_KEYS, WINDOW_SIZE> window;

    template <typename T>
    void operator()(const data_t in, fx::FlatMapShipper<T> & shipper) {
    #pragma HLS INLINE
        const unsigned int key = in.key;
        const IN_T value = in.value;
        const unsigned int timestamp = in.timestamp;

        OUT_T agg = OP::lower(OP::identity());
        bool valid = window.update(key, value, agg);

        if (valid) {
            data_t out;
            out.key = key;
            out.value = value;
            out.aggregate = agg;
            out.timestamp = timestamp;
            shipper.send(out);
        }
    }
};

void test(in_stream_t & in, out_stream_t & out)
{
    using OP = fx::Sum<float>;
    static constexpr unsigned int L = OP::LATENCY;

    #pragma HLS DATAFLOW

    fx::FlatMap<KeyedCountTumblingWindowFunctor<OP>, L>(
        in, out
    );
}
#endif