#ifndef __COMMON__HPP__
#define __COMMON__HPP__

#include <iostream>
#include <type_traits>
#include <limits>

#define UNUSED(x) (void)(x)

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

TEMPLATE_INTEGRAL
constexpr T POW2(T v) {
    return 1 << v;
}

TEMPLATE_INTEGRAL
constexpr T POW2_CEIL(T v) {
    return IS_POW2(v) ? v : POW2(LOG2(v) + 1);
}

TEMPLATE_INTEGRAL
constexpr T POW2_FLOOR(T v) {
    return IS_POW2(v) ? v : POW2(LOG2(v));
}

TEMPLATE_INTEGRAL
constexpr T DIV_CEIL(T a, T b) {
    return (a + b - 1) / b;
}

TEMPLATE_INTEGRAL
constexpr T DIV_FLOOR(T a, T b) {
    return a / b;
}

TEMPLATE_FLOATING
ALWAYS_INLINE bool approximatelyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
{
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}

#endif // __COMMON_HPP__