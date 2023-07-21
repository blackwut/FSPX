#ifndef __CONSTANTS_HPP__
#define __CONSTANTS_HPP__

#include "record_t.hpp"

static constexpr int MAX_KEY_VALUE = 63;
static constexpr int MAX_KEYS = MAX_KEY_VALUE + 1;
static constexpr int WIN_SIZE = 16;

template <typename float_t>
static constexpr float_t THRESHOLD = float_t(0.025);

#define LOAD_REAL_DATASET 0
static constexpr int TUPLES_PER_KEY = 16 * WIN_SIZE;


#endif // __CONSTANTS_HPP__