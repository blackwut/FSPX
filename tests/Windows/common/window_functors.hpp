#ifndef __WINDOW_FUNCTORS_HPP__
#define __WINDOW_FUNCTORS_HPP__

#include <iostream>
#include <iomanip>
#include "fspx.hpp"

#include "tuples.hpp"

struct window_functor_count
{
    void operator()(const tuple_t & tuple, result_count_t & result)
    {
    #pragma HLS INLINE
        UNUSED(tuple);
        result.count = result.count + 1;
    }
};

struct window_functor_mean
{
    void operator()(const tuple_t & tuple, result_mean_t & result)
    {
    #pragma HLS INLINE
        result.sum += tuple.value;
        result.count = result.count + 1;
    }
};

#endif // __WINDOW_FUNCTORS_HPP__