#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <iostream>

#include "constants.hpp"
#include "tuple.hpp"

static record_t new_record(int i)
{
    record_t r;
    r.key = static_cast<unsigned int>(i % record_t::MAX_KEY_VALUE);
    r.property_value = static_cast<float>(i);
    r.incremental_average = 0;
    r.timestamp = 0;
    return r;
}

static record_t new_record_spike(int i, int remainder = 32)
{
    record_t r;
    r.key = static_cast<unsigned int>(1);
    r.property_value = ((i + 1) % remainder) ? static_cast<float>(10) : 12;
    r.incremental_average = 0;
    r.timestamp = 0;
    return r;
}

static void print_record(const record_t & r)
{
    std::cout << "("
    << r.key << ", "
    << r.property_value << ", "
    << r.incremental_average << ", "
    << r.timestamp
    << ")"
    << std::endl;
}

static line_t new_line(int i)
{
    line_t line = 0;
    for (int j = 0; j < READ_WRITE_ITEMS; ++j) {
        record_t r = new_record(i * READ_WRITE_ITEMS + j);
        line.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j) = TypeHandler<record_t>::to_ap(r);
    }
    return line;
}

static void print_line(const line_t l)
{
    for (int j = 0; j < READ_WRITE_ITEMS; ++j) {
        ap_uint<ITEM_BITS> item = l.range(ITEM_BITS * (j + 1) - 1, ITEM_BITS * j);
        record_t r = TypeHandler<record_t>::from_ap(item);
        print_record(r);
    }
}

#endif // __UTILS_HPP__