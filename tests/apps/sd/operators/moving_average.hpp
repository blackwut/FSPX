#ifndef __MOVING_AVERAGE_HPP__
#define __MOVING_AVERAGE_HPP__

#include "common/constants.hpp"
#include "tuples/record_t.hpp"

template <typename T, int SIZE>
struct window_t
{
    T win[SIZE];

    window_t()
    : win{}
    {
    #pragma HLS array_partition variable=win complete dim=1
    }

    T update(T val)
    {
    #pragma HLS INLINE
    T sum = 0;
    WIN_SHIFT:
        for (int i = 0; i < SIZE - 1; ++i) {
        #pragma HLS unroll
            win[i] = win[i + 1];
            sum += win[i];
        }
        win[SIZE - 1] = val;
        sum += val;
        return sum;
    }
};

template <typename float_t>
struct MovingAverage
{
    unsigned int sizes[MAX_KEYS];
    window_t<float_t, WIN_SIZE> windows[MAX_KEYS];

    MovingAverage()
    : sizes{}
    {
        #pragma HLS INLINE
        #pragma HLS array_partition variable=sizes type=complete dim=1
    }

    void operator() (record_t in, record_t & out) {
    #pragma HLS INLINE

        const unsigned int idx = in.key;

        float_t N = float_t(1) / WIN_SIZE;
        if (sizes[idx] < WIN_SIZE) {
            N = 1.0f / ++(sizes[idx]);
        }

        const float_t sum = windows[idx].update(in.property_value);
        out.key = in.key;
        out.property_value = in.property_value;
        out.incremental_average = sum * N;
        out.timestamp = in.timestamp;
    }
};

#endif // __MOVING_AVERAGE_HPP__