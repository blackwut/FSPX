#ifndef __AGGREGATE_OPERATORS_HPP__
#define __AGGREGATE_OPERATORS_HPP__

#include "hls_math.h"
#include <limits>

namespace fx {

template <typename T, typename COUNT_T = unsigned int>
struct Count {
    using IN_T = T;
    using AGG_T = COUNT_T;
    using OUT_T = COUNT_T;

    static constexpr AGG_T identity() {
        return 0;
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return 1;
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return a + b;
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a;
    }
};

template <typename T>
struct Sum {
    using IN_T = T;
    using AGG_T = T;
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return 0;
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return a;
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return a + b;
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a;
    }
};

template <typename T>
struct Max {
    using IN_T = T;
    using AGG_T = T;
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return std::numeric_limits<T>::lowest();
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return a;
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return (a > b) ? a : b;
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a;
    }
};

template <typename T>
struct Min {
    using IN_T = T;
    using AGG_T = T;
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return std::numeric_limits<T>::upper();
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return a;
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return (a < b) ? a : b;
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a;
    }
};

template <typename T, typename COUNT_T = unsigned int, typename RESULT_T = float>
struct ArithmeticMean {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T sum; };
    using OUT_T = RESULT_T;

    static constexpr AGG_T identity() {
        return {0, 0};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return {a.count + b.count, a.sum + b.sum};
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a.sum / OUT_T(a.count);
    }
};

template <typename T, typename COUNT_T = unsigned int, typename RESULT_T = float>
struct GeometricMean {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T product; };
    using OUT_T = RESULT_T;

    static constexpr AGG_T identity() {
        return {0, 1};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return {a.count + b.count, a.product * b.product};
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return pow(a.product, 1.0f / a.count);
    }
};

template <typename T, typename COUNT_T>
struct MaxCount {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T max; };
    using OUT_T = COUNT_T;

    static constexpr AGG_T identity() {
        return {0, std::numeric_limits<T>::lower()};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        if (a.max > b.max) {
            return a;
        } else if (a.max < b.max) {
            return b;
        } else {
            return {a.count + b.count, a.max};
        }
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a.count;
    }
};

template <typename T, typename COUNT_T>
struct MinCount {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T min; };
    using OUT_T = COUNT_T;

    static constexpr AGG_T identity() {
        return {0, std::numeric_limits<T>::upper()};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        if (a.min < b.min) {
            return a;
        } else if (a.min > b.min) {
            return b;
        } else {
            return {a.count + b.count, a.min};
        }
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return a.count;
    }
};

template <typename T, typename COUNT_T = unsigned int>
struct SampleStdDev {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T sum; T sq; };
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return {0, 0, 0};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a, a * a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return {a.count + b.count, a.sum + b.sum, a.sq + b.sq};
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return sqrt((a.sq - (a.sum * a.sum) / a.count) / (a.count - 1));
    }
};

template <typename T, typename COUNT_T = unsigned int>
struct PopulationStdDev {
    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T sum; T sq; };
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return {0, 0, 0};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {1, a, a * a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return {a.count + b.count, a.sum + b.sum, a.sq + b.sq};
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return sqrt((a.sq - (a.sum * a.sum) / a.count) / a.count);
    }
};

// TODO: ArgMax
// TODO: ArgMin
// TODO: find other useful aggregate operators

} // namespace fx

#endif // __AGGREGATE_OPERATORS_HPP__