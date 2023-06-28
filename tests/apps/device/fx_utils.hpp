#ifndef __FX_UTILS_HPP__
#define __FX_UTILS_HPP__

#include <type_traits>
#include <limits>
#include <cstdint>
#include <cmath>
#include <random>
#include <sys/time.h>
#include <iostream>
#include <iomanip>

#define ALLOC_ALIGNMENT 4096

template <typename T>
T * aligned_alloc(const size_t n)
{
    T * ptr = NULL;
    int ret = posix_memalign((void **)&ptr, ALLOC_ALIGNMENT, n * sizeof(T));
    if (ret != 0) {
        exit(ret);
    }
    return ptr;
}

template <typename T>
struct aligned_allocator {
    using value_type = T;
    T * allocate(std::size_t num) {
        void * ptr = nullptr;
        if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
        return reinterpret_cast<T*>(ptr);
    }
    void deallocate(T* p, std::size_t num) { free(p); }
};

#define COUT_STRING_W           24
#define COUT_DOUBLE_W           8
#define COUT_INTEGER_W          8
#define COUT_PRECISION          3

#define COUT_HEADER             std::setw(COUT_STRING_W) << std::right
#define COUT_FLOAT              std::setw(COUT_DOUBLE_W) << std::right << std::fixed << std::setprecision(COUT_PRECISION)
#define COUT_INTEGER            std::setw(COUT_INTEGER_W) << std::right << std::fixed
#define COUT_BOOLEAN            std::boolalpha << std::right

#define ALWAYS_INLINE           inline __attribute__((always_inline))
#define TEMPLATE_FLOATING       template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL       template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL_(x)   template<typename T, typename std::enable_if<std::is_integral<T>::value && (sizeof(T) == x), bool>::type = true>
#define TEMPLATE_INTEGRAL_32    TEMPLATE_INTEGRAL_(4)
#define TEMPLATE_INTEGRAL_64    TEMPLATE_INTEGRAL_(8)

ALWAYS_INLINE uint64_t current_time_ns()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec) * uint64_t(1000000000) + t.tv_nsec;
}

ALWAYS_INLINE uint64_t current_time_us()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec) * uint64_t(1000000) + t.tv_nsec / uint64_t(1000);
}

ALWAYS_INLINE uint64_t current_time_ms()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec) * uint64_t(1000) + t.tv_nsec / uint64_t(1000000);
}

TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_ns_to_us(const T time) { return time / T(1000); }

TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_ns_to_ms(const T time) { return time / T(1000000); }

TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_ns_to_s(const T time) { return time / T(1000000000); }

// uint32_t max 4294967.296 microseconds
TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_us_to_ns(const T time) { return time * T(1000); }

// uint32_t max 4294.967296 milliseconds
TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_ms_to_ns(const T time) { return time * T(1000000); }

// uint32_t max 4.294967296 seconds
TEMPLATE_INTEGRAL
ALWAYS_INLINE T time_s_to_ns(const T time) { return time * T(1000000000); }

TEMPLATE_INTEGRAL
ALWAYS_INLINE T divide_up(T size, T div)
{
    return (size + div - 1) / div;
}

TEMPLATE_INTEGRAL
ALWAYS_INLINE T round_up(T size, T div)
{
    return divide_up(size, div) * div;
}


TEMPLATE_INTEGRAL_32
ALWAYS_INLINE T next_pow2(T v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    v += (v == 0);
    return v;
}

TEMPLATE_INTEGRAL_64
ALWAYS_INLINE T next_pow2(T v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    v += (v == 0);
    return v;
}


TEMPLATE_INTEGRAL
ALWAYS_INLINE T log_2(T v)
{
    T r = 0;
    while (v >>= 1) { r++; }
    return r;
}


TEMPLATE_FLOATING
ALWAYS_INLINE bool approximatelyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
{
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}


TEMPLATE_FLOATING
ALWAYS_INLINE bool essentiallyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
{
    return std::abs(a - b) <= ( (std::abs(a) > std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}


TEMPLATE_FLOATING
ALWAYS_INLINE T next_float(const T from, const T to)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<T> dist(from, to);

    return dist(gen);
}

#endif // __FX_UTILS_HPP__
