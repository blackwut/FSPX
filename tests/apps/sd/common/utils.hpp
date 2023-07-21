#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <iostream>

#include "constants.hpp"
#include "tuples/record_t.hpp"

static record_t new_record(int i)
{
    record_t r;
    r.key = static_cast<unsigned int>(i % (MAX_KEY_VALUE + 1));
    r.property_value = static_cast<float>(i);
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
        << '\n';
}

#endif // __UTILS_HPP__