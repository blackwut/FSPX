#ifndef __MOVING_AVERAGE_HPP__
#define __MOVING_AVERAGE_HPP__

#include "tuple.hpp"

template <typename T, int SIZE>
struct window_t
{
    T win[SIZE];

    window_t()
    : win{}
    {
    #pragma HLS ARRAY_PARTITION variable=win complete dim=0
    }

    T update(T val)
    {
    WIN_SHIFT:
        T sum = 0;
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
    static constexpr int MAX_KEYS = record_t::MAX_KEY_VALUE;
    static constexpr int WIN_SIZE = 16;

    unsigned int sizes[MAX_KEYS];
    window_t<float_t, WIN_SIZE> windows[MAX_KEYS];

    MovingAverage()
    : sizes{}
    {
        #pragma HLS array_partition variable=sizes type=complete dim=0
    }

    record_t operator() (record_t in) {
    #pragma HLS INLINE

        const unsigned int idx = in.key;

        float_t N = float_t(1) / WIN_SIZE;
        if (sizes[idx] < WIN_SIZE) {
            N = 1.0f / ++(sizes[idx]);
        }

        const float_t sum = windows[idx].update(in.property_value);
        record_t out;
        out.key = in.key;
        out.property_value = in.property_value;
        out.incremental_average = sum * N;
        out.timestamp = in.timestamp;
        return out;
    }
};

#endif // __MOVING_AVERAGE_HPP__