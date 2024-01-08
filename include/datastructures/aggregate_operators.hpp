#ifndef __AGGREGATE_OPERATORS_HPP__
#define __AGGREGATE_OPERATORS_HPP__

#include "hls_math.h"
#include <limits>

namespace fx {

template <typename T>
struct OperatorLatency
{
    static constexpr unsigned int Count = 1;
    static constexpr unsigned int Sum = 1;
    static constexpr unsigned int Max = 1;
    static constexpr unsigned int Min = 1;
    static constexpr unsigned int ArithmeticMean = 1;
    static constexpr unsigned int GeometricMean = 1;
    static constexpr unsigned int MaxCount = 1;
    static constexpr unsigned int MinCount = 1;
    static constexpr unsigned int SampleStdDev = 1;
    static constexpr unsigned int PopulationStdDev = 1;
};

template <>
struct OperatorLatency<float>
{
    static constexpr unsigned int Count = 1;
    static constexpr unsigned int Sum = 4;
    static constexpr unsigned int Max = 1;
    static constexpr unsigned int Min = 1;
    static constexpr unsigned int ArithmeticMean = 4;
    static constexpr unsigned int GeometricMean = 4;
    static constexpr unsigned int MaxCount = 1;
    static constexpr unsigned int MinCount = 1;
    static constexpr unsigned int SampleStdDev = 4;
    static constexpr unsigned int PopulationStdDev = 4;
};

template <>
struct OperatorLatency<double>
{
    // static constexpr unsigned int Count = 1;
    // static constexpr unsigned int Sum = 4;
    // static constexpr unsigned int Max = 1;
    // static constexpr unsigned int Min = 1;
    // static constexpr unsigned int ArithmeticMean = 4;
    // static constexpr unsigned int GeometricMean = 4;
    // static constexpr unsigned int MaxCount = 1;
    // static constexpr unsigned int MinCount = 1;
    // static constexpr unsigned int SampleStdDev = 4;
    // static constexpr unsigned int PopulationStdDev = 4;
    // static constexpr unsigned int MinMax = 1;
};

template <
    typename T,
    typename COUNT_T = unsigned int,
    unsigned int L = OperatorLatency<COUNT_T>::Count
>
struct Count {
    static constexpr unsigned int LATENCY = L;

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

template <
    typename T,
    unsigned int L = OperatorLatency<T>::Sum
>
struct Sum {
    static constexpr unsigned int LATENCY = L;

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

template <
    typename T,
    unsigned int L = OperatorLatency<T>::Max
>
struct Max {
    static constexpr unsigned int LATENCY = L;

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

template <
    typename T,
    unsigned int L = OperatorLatency<T>::Min
>
struct Min {
    static constexpr unsigned int LATENCY = L;

    using IN_T = T;
    using AGG_T = T;
    using OUT_T = T;

    static constexpr AGG_T identity() {
        return std::numeric_limits<T>::max();
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

template <
    typename T,
    typename COUNT_T = unsigned int,
    typename RESULT_T = float,
    unsigned int L = OperatorLatency<T>::ArithmeticMean
>
struct ArithmeticMean {
    static constexpr unsigned int LATENCY = L;

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

template <
    typename T,
    typename COUNT_T = unsigned int,
    typename RESULT_T = float,
    unsigned int L = OperatorLatency<T>::GeometricMean
>
struct GeometricMean {
    static constexpr unsigned int LATENCY = L;

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
        return hls::pow(a.product, 1.0f / a.count);
    }
};

template <
    typename T,
    typename COUNT_T = unsigned int,
    unsigned int L = OperatorLatency<T>::MaxCount
>
struct MaxCount {
    static constexpr unsigned int LATENCY = L;

    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T max; };
    using OUT_T = COUNT_T;

    static constexpr AGG_T identity() {
        return {0, std::numeric_limits<T>::lowest()};
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

template <
    typename T,
    typename COUNT_T = unsigned int,
    unsigned int L = OperatorLatency<T>::MinCount
>
struct MinCount {
    static constexpr unsigned int LATENCY = L;

    using IN_T = T;
    using AGG_T = struct { COUNT_T count; T min; };
    using OUT_T = COUNT_T;

    static constexpr AGG_T identity() {
        return {0, std::numeric_limits<T>::max()};
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

template <
    typename T,
    typename COUNT_T = unsigned int,
    unsigned int L = OperatorLatency<T>::SampleStdDev
>
struct SampleStdDev {
    static constexpr unsigned int LATENCY = L;

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
        return hls::sqrt((a.sq - (a.sum * a.sum) / a.count) / (a.count - 1));
    }
};

template <
    typename T,
    typename COUNT_T = unsigned int,
    unsigned int L = OperatorLatency<T>::PopulationStdDev
>
struct PopulationStdDev {
    static constexpr unsigned int LATENCY = L;

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
        return hls::sqrt((a.sq - (a.sum * a.sum) / a.count) / a.count);
    }
};


// TODO: ArgMax
// TODO: ArgMin


// Example of composite operator
template <
    typename T,
    unsigned int L = 1// OperatorLatency<T>::MinMax
>
struct MinMax {
    static constexpr unsigned int LATENCY = L;

    using IN_T = T;
    using AGG_T = struct { T min; T max; };
    using OUT_T = struct { T min; T max; };

    static constexpr AGG_T identity() {
        return {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()};
    }

    static AGG_T lift(const IN_T & a) {
    #pragma HLS INLINE
        return {a, a};
    }

    static AGG_T combine(const AGG_T & a, const AGG_T & b) {
    #pragma HLS INLINE
        return {a.min < b.min ? a.min : b.min, a.max > b.max ? a.max : b.max};
    }

    static OUT_T lower(const AGG_T & a) {
    #pragma HLS INLINE
        return {a.min, a.max};
    }
};

} // namespace fx

#endif // __AGGREGATE_OPERATORS_HPP__