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

bool update_last(bool * lasts, int size)
{
    bool last = true;
    for (int j = 0; j < size; ++j) {
        last &= lasts[j];
    }
    return last;
}

int main() {

    line_t in[SIZE];
    stream_t outs[READ_ITEMS];

    for (int i = 0; i < SIZE; ++i) {
        in[i] = new_line(i);
    }

    test(in, outs, SIZE);

    int i = 0;
    bool lasts[READ_ITEMS];
    for (int j = 0; j < READ_ITEMS; ++j) {
        lasts[j] = outs[j].read_eos();
    }
    bool last = update_last(lasts, READ_ITEMS);

    while (!last) {
        line_t line = in[i];
        for (int j = 0; j < READ_ITEMS; ++j) {
            data_t r = outs[j].read();
            lasts[j] = outs[j].read_eos();

            ap_uint<ITEM_BITS> item = line.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j);
            data_t d = TypeHandler<data_t>::from_ap(item);
            // print_data(r);

            if (d != r) {
                return 1;
            }
        }

        last = update_last(lasts, READ_ITEMS);
        ++i;
    }

    return 0;
}
