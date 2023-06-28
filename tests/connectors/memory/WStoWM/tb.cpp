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

    stream_t in("in");
    line_t out[SIZE];
    int out_size = sizeof(out);

    for (int i = 0; i < SIZE; ++i) {
        line_t l = new_line(i);
        // print_line(l);
        in.write(l);
    }
    in.write_eos();

    int write_count;
    int eos;

    test(in, out, out_size, &write_count, &eos);

    std::cout << "write_count: " << write_count << std::endl;
    std::cout << "eos: " << (eos != 0 ? "true" : "false") << std::endl;

    if (write_count != SIZE) {
        return 1;
    }

    if (eos != 1) {
        return 2;
    }

    for (int i = 0; i < write_count; ++i) {
        line_t l = new_line(i);
        // print_line(out[i]);
        if (out[i] != l) {
            return 3;
        }
    }

    return 0;
}
