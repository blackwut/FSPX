#ifndef __COMMON__HPP__
#define __COMMON__HPP__

#include <iostream>
#include <type_traits>
#include <limits>
#include "ap_int.h"

#define UNUSED(x) (void)(x)
#define REMOVE_INIT(x) union { x; }

#if !defined(__SYNTHESIS__)
#include <stdio.h>
#define hls_print(x, y) printf(x, y)
#else 
#include "hls_print.h"
#define hls_print(x, y) hls::print(x, y)
#endif

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
#define TEMPLATE_INTEGRAL2      template<typename T, typename U, typename std::enable_if<std::is_integral<T>::value, bool>::type = true, typename std::enable_if<std::is_integral<U>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL_(x)   template<typename T, typename std::enable_if<std::is_integral<T>::value && (sizeof(T) == x), bool>::type = true>
#define TEMPLATE_INTEGRAL_32    TEMPLATE_INTEGRAL_(4)
#define TEMPLATE_INTEGRAL_64    TEMPLATE_INTEGRAL_(8)

TEMPLATE_INTEGRAL
constexpr bool IS_POW2(T v) {
    return (v > 0) && v && !(v & (v - 1));
}

TEMPLATE_INTEGRAL
constexpr T LOG2_FLOOR(T val) {
  return val <= 1 ? 0 : 1 + LOG2_FLOOR(val >> 1);
}

TEMPLATE_INTEGRAL
constexpr T LOG2_CEIL(T n)
{
    return (n <= 1) ? 0 : 1 + LOG2_FLOOR(n - 1);
}

TEMPLATE_INTEGRAL
constexpr T POW2(T v) {
    return 1 << v;
}

TEMPLATE_INTEGRAL
constexpr T POW2_CEIL(T v) {
    return IS_POW2(v) ? v : POW2(LOG2_FLOOR(v) + 1);
}

TEMPLATE_INTEGRAL
constexpr T POW2_FLOOR(T v) {
    return IS_POW2(v) ? v : POW2(LOG2_FLOOR(v));
}

TEMPLATE_INTEGRAL
constexpr T DIV_CEIL(T a, T b) {
    return (a + b - 1) / b;
}

TEMPLATE_INTEGRAL
constexpr T DIV_FLOOR(T a, T b) {
    return a / b;
}

TEMPLATE_INTEGRAL2
constexpr U DIV_CEIL(T a, U b) {
    return (a + b - 1) / b;
}

TEMPLATE_INTEGRAL2
constexpr U DIV_FLOOR(T a, U b) {
    return a / b;
}

TEMPLATE_INTEGRAL
constexpr T MIN_VAL(T a, T b) {
    return (a < b) ? a : b;
}

TEMPLATE_INTEGRAL
constexpr T MAX_VAL(T a, T b) {
    return (a > b) ? a : b;
}

TEMPLATE_FLOATING
ALWAYS_INLINE bool approximatelyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon()) {
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * epsilon);
}

template <typename T>
void print_array(std::string name, const T * array, const unsigned int size)
{
    std::cout << name << ": ";
    for (unsigned int i = 0; i < size; ++i) {
        std::cout << array[i];
        if (i < size - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
}

// template <int N>
// void print_bits(const ap_uint<N> val, const std::string message)
// {
//     std::cout << message << ": ";
//     for (int i = N - 1; i >= 0; --i) {
//         std::cout << val[i];
//     }
//     std::cout << std::endl;
// }

//******************************************************************************
//
// Types
//
//******************************************************************************

namespace fx {

template <unsigned int MAX_VALUE>
using uint_for = ap_uint<LOG2_CEIL(MAX_VALUE) + (MAX_VALUE == 1)>;

template <unsigned int MAX_VALUE>
std::enable_if_t<IS_POW2<MAX_VALUE>, void>
increment(uint_for<MAX_VALUE> & value) {
    value = value + 1;
}

template <unsigned int MAX_VALUE>
std::enable_if_t<!IS_POW2<MAX_VALUE>, void>
increment(uint_for<MAX_VALUE> & value) {
    using T = uint_for<MAX_VALUE>;
    value = (T(value + 1) == T(MAX_VALUE)) ? T(0) : T(value + 1);
}

using index_t = unsigned long;

using key_t = unsigned long;
using timestamp_t = unsigned long;

using wid_t = unsigned int;
using seq_t = unsigned long;

} // namespace fx

//******************************************************************************
//
// TypeTraits
//
//******************************************************************************

template<std::size_t I, class T>
struct tuple_elem;
 
// recursive case
template<std::size_t I, class Head, class... Tail>
struct tuple_elem<I, std::tuple<Head, Tail...>>
    : tuple_elem<I - 1, std::tuple<Tail...>>
{};
 
// base case
template<class Head, class... Tail>
struct tuple_elem<0, std::tuple<Head, Tail...>> {
    using type = Head;
};

template <typename T>
struct remove_cvref_t {
    using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
};

// https://stackoverflow.com/a/78113051
template<typename Result, typename ...Args>  struct FunctionSignatureParserBase
{
    using return_type = Result;
    using args_tuple  = std::tuple<Args...>;
    template <std::size_t i>
    struct arg
    {
        typedef typename std::decay_t<typename tuple_elem<i, args_tuple>::type> type;
    };

    template <std::size_t i>  using arg_t = typename arg<i>::type;
};

template<typename T>  struct FunctionSignatureParser;

template<typename ClassType, typename Result, typename...Args>
struct FunctionSignatureParser<Result(ClassType::*)(Args...)>
  : FunctionSignatureParserBase<Result,Args...>
{};

template<typename ClassType, typename Result, typename...Args>
struct FunctionSignatureParser<Result(ClassType::*)(Args...) const>
  : FunctionSignatureParserBase<Result,Args...>
{};

// get arg<n> type
template<std::size_t n, typename T>
using _arg_t = typename FunctionSignatureParser<T>::template arg_t<n>;
#define arg_t(n, f) _arg_t<n, decltype(&f)>

// get return type
template<typename T>
using _return_t = typename FunctionSignatureParser<T>::return_type;
#define return_t(f) _return_t<decltype(&f)>

#endif // __COMMON_HPP__
