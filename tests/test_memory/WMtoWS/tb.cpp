#include "kernel.hpp"

data_t new_data(int i)
{
    data_t d;
    d.x[0] = i >> 24;
    d.x[1] = i >> 16;
    d.x[2] = i >> 8;
    d.x[3] = i;
    d.y[0] = i >> 16;
    d.y[1] = i;
    d.z = i;
    return d;
}

void print_data(const data_t & d)
{
    std::cout << "("
    << (int)d.x[0] << ", "
    << (int)d.x[1] << ", "
    << (int)d.x[2] << ", "
    << (int)d.x[3] << ", "
    << (int)d.y[0] << ", "
    << (int)d.y[1] << ", "
    << d.z
    << ")"
    << std::endl;
}

line_t new_line(int i)
{
    line_t line = 0;
    for (int j = 0; j < READ_ITEMS; ++j) {
        data_t d = new_data(i * READ_ITEMS + j);
        line.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j) = TypeHandler<data_t>::to_ap(d);
    }
    return line;
}

void print_line(const line_t l)
{
    for (int j = 0; j < READ_ITEMS; ++j) {
        ap_uint<ITEM_BITS> item = l.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j);
        data_t d = TypeHandler<data_t>::from_ap(item);
        print_data(d);
    }
}

int main() {

    line_t in[SIZE];
    stream_t out("out");

    for (int i = 0; i < SIZE; ++i) {
        in[i] = new_line(i);
    }

    test(in, out, SIZE);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        line_t l = out.read();
        last = out.read_eos();

        // print_line(l);
        // for (int j = 0; j < write_items; ++j) {
        //     ap_uint<ITEM_BITS> item = l.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j);
        //     data_t d = TypeHandler<data_t>::from_ap(item);
        //     print_data(d);
        // }

        if (in[i] != l) {
            return 1;
        }
        ++i;
    }

    return 0;
}
