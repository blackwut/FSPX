#ifndef __COMMON__HPP__
#define __COMMON__HPP__

#include <type_traits>
#include <limits>

#define UNUSED(x) (void)(x)

#include <iostream>
#define DUMP_VAR(x) { std::cout << #x << ": " << x << std::endl; }
#define DUMP_MSG_VAR(msg, x) { std::cout << msg << ": " << x << std::endl; }


// #ifndef __SYNTHESIS__
// #include <iostream>
// #define DUMP_VAR(x)             { std::cout << #x  << ": " << x << std::endl; }
// #define DUMP_MSG_VAR(msg, x)    { std::cout << msg << ": " << x << std::endl; }
// #else
// #define DUMP_VAR(x)          ((void)0)
// #define DUMP_MSG_VAR(msg, x) ((void)0)
// #endif

#ifndef __SYNTHESIS__
#include <cassert>
#define HW_ASSERT(b) assert((b))
#else
#define HW_ASSERT(b) ((void)0)
#endif

#if __cplusplus >= 201103L
#define HW_STATIC_ASSERT(b, m) static_assert((b), m)
#else
#define HW_STATIC_ASSERT(b, m) HW_ASSERT((b) && (m))
#endif

#define ALWAYS_INLINE           inline __attribute__((always_inline))
#define TEMPLATE_FLOATING       template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL       template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL_(x)   template<typename T, typename std::enable_if<std::is_integral<T>::value && (sizeof(T) == x), bool>::type = true>
#define TEMPLATE_INTEGRAL_32    TEMPLATE_INTEGRAL_(4)
#define TEMPLATE_INTEGRAL_64    TEMPLATE_INTEGRAL_(8)

TEMPLATE_INTEGRAL
constexpr T LOG2(T n)
{
  return (n < 2) ? 1 : 1 + LOG2(n / 2);
}

TEMPLATE_INTEGRAL
constexpr bool IS_POW2(T v) {
    return (v > 0) && v && !(v & (v - 1));
}

TEMPLATE_FLOATING
ALWAYS_INLINE bool approximatelyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
{
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}

// https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/DataPack.h#L27
template <typename T>
struct TypeHandler {
    static constexpr int WIDTH = 8 * sizeof(T);

    static T from_ap(ap_uint<WIDTH> const & ap) {
        return *reinterpret_cast<T const *>(&ap);
    }

    static ap_uint<WIDTH> to_ap(T const & value) {
        return *reinterpret_cast<ap_uint<WIDTH> const *>(&value);
    }
};


// https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/DataPack.h#L113
template <typename T, int N>
class DataPack
{
    static_assert(N > 0, "Width must be positive");

public:

    static constexpr int WIDTH = TypeHandler<T>::WIDTH;
    using item_t = ap_uint<WIDTH>;
    using items_t = ap_uint<N * WIDTH>;

    DataPack() : items_() {}

    DataPack(DataPack<T, N> const & other) = default;

    DataPack(DataPack<T, N> && other) = default;

    DataPack(T const & value) : items_()
    {
    #pragma HLS INLINE
        fill(value);
    }

    explicit DataPack(T const arr[N])
    : items_()
    {
    #pragma HLS INLINE
        pack(arr);
    }

    DataPack<T, N>& operator=(DataPack<T, N> && other)
    {
    #pragma HLS INLINE
        items_ = other.items_;
        return *this;
    }

    DataPack<T, N>& operator=(DataPack<T, N> const &other)
    {
    #pragma HLS INLINE
        items_ = other.items_;
        return *this;
    }

    T get(int i) const
    {
    #pragma HLS INLINE
    // #ifndef __SYNTHESIS__
    //     if (i < 0 || i >= N) {
    //         std::stringstream ss;
    //         ss << "Index " << i << " out of range for DataPack of " << N << " items";
    //         throw std::out_of_range(ss.str());
    //     }
    // #endif
        item_t temp = items_.range((i + 1) * WIDTH - 1, i * WIDTH);
        return TypeHandler<T>::from_ap(temp);
    }

    void set(int i, T value)
    {
    #pragma HLS INLINE
    // #ifndef __SYNTHESIS__
    //     if (i < 0 || i >= N) {
    //         std::stringstream ss;
    //         ss << "Index " << i << " out of range for DataPack of " << N << " items";
    //         throw std::out_of_range(ss.str());
    //     }
    // #endif
        items_.range((i + 1) * WIDTH - 1, i * WIDTH) = (
            TypeHandler<T>::to_ap(value)
        );
    }

    void fill(T const & value)
    {
    #pragma HLS INLINE
    DataPack_fill:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            set(i, value);
        }
    }

    void pack(T const arr[N])
    {
    #pragma HLS INLINE
    DataPack_pack:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            set(i, arr[i]);
        }
    }

    void unpack(T arr[N]) const
    {
    #pragma HLS INLINE
    DataPack_Unpack:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            arr[i] = get(i);
        }
    }

    T operator[](const int i) const {
    #pragma HLS INLINE
        return get(i);
    }

  // Access to internal data directly if necessary
  items_t & data() { return items_; }
  items_t data() const { return items_; }

 private:

  items_t items_;
};

#endif // __COMMON_HPP__