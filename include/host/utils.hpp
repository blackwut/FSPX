#ifndef __HOST_UTILS__
#define __HOST_UTILS__

#include <type_traits>
#include <limits>
#include <cstdint>
#include <cmath>
#include <random>
#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#define ALLOC_ALIGNMENT 4096

#define UNUSED(x) (void)(x)

namespace fx {

template <typename T>
T * aligned_alloc(const size_t n)
{
    void * ptr = nullptr;
    int ret = posix_memalign((void **)&ptr, ALLOC_ALIGNMENT, n * sizeof(T));
    if (ret != 0) {
        exit(ret);
    }
    return reinterpret_cast<T *>(ptr);
}

template <typename T>
struct aligned_allocator {
    using value_type = T;
    T * allocate(std::size_t num) {
        void * ptr = nullptr;
        if (posix_memalign(&ptr, ALLOC_ALIGNMENT, num * sizeof(T))) throw std::bad_alloc();
        return reinterpret_cast<T*>(ptr);
    }
    void deallocate(T* p, std::size_t num) { free(p); }
};

#define COUT_STRING_W           24
#define COUT_STRING_SMALL_W     16
#define COUT_DOUBLE_W           8
#define COUT_INTEGER_W          8
#define COUT_PRECISION          3

#define COUT_HEADER             std::setw(COUT_STRING_W) << std::right
#define COUT_HEADER_SMALL       std::setw(COUT_STRING_SMALL_W) << std::right
#define COUT_FLOAT              std::setw(COUT_DOUBLE_W) << std::right << std::fixed << std::setprecision(COUT_PRECISION)
#define COUT_FLOAT_(x)          std::setw(COUT_DOUBLE_W) << std::right << std::fixed << std::setprecision(x)
#define COUT_INTEGER            std::setw(COUT_INTEGER_W) << std::right << std::fixed
#define COUT_BOOLEAN            std::boolalpha << std::right

#define ALWAYS_INLINE           inline __attribute__((always_inline))
#define TEMPLATE_FLOATING       template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL       template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
#define TEMPLATE_INTEGRAL_(x)   template<typename T, typename std::enable_if<std::is_integral<T>::value && (sizeof(T) == x), bool>::type = true>
#define TEMPLATE_INTEGRAL_32    TEMPLATE_INTEGRAL_(4)
#define TEMPLATE_INTEGRAL_64    TEMPLATE_INTEGRAL_(8)


const std::string COLOR_RESET = "\033[0m";
const std::string COLOR_BLACK = "\033[30m";
const std::string COLOR_RED = "\033[31m";
const std::string COLOR_GREEN = "\033[32m";
const std::string COLOR_YELLOW = "\033[33m";
const std::string COLOR_BLUE = "\033[34m";
const std::string COLOR_MAGENTA = "\033[35m";
const std::string COLOR_CYAN = "\033[36m";
const std::string COLOR_WHITE = "\033[37m";
const std::string COLOR_GRAY = "\033[90m";
const std::string COLOR_LIGHT_RED = "\033[91m";
const std::string COLOR_LIGHT_GREEN = "\033[92m";
const std::string COLOR_LIGHT_YELLOW = "\033[93m";
const std::string COLOR_LIGHT_BLUE = "\033[94m";
const std::string COLOR_LIGHT_MAGENTA = "\033[95m";
const std::string COLOR_LIGHT_CYAN = "\033[96m";
const std::string COLOR_LIGHT_WHITE = "\033[97m";

ALWAYS_INLINE std::string formatBytesPerSecond(size_t bytesPerSecond)
{
    static const char * suffixes[] = {"B/s", "KB/s", "MB/s", "GB/s", "TB/s"};

    if (bytesPerSecond == 0) {
        return "0 B/s";
    }

    size_t suffixIndex = 0;
    double speed = static_cast<double>(bytesPerSecond);

    while (speed >= 1024 && suffixIndex < (sizeof(suffixes) / sizeof(suffixes[0])) - 1) {
        speed /= 1024;
        ++suffixIndex;
    }

    std::stringstream stream;
    stream << COUT_FLOAT_(2) << speed << " " << suffixes[suffixIndex];

    return stream.str();
}

ALWAYS_INLINE std::string formatTuplesPerSecond(size_t tuplesPerSecond)
{
    static const char* suffixes[] = {"t/s", "kt/s", "Mt/s", "Gt/s", "Tt/s"};

    if (tuplesPerSecond == 0) {
        return "0 t/s";
    }

    size_t suffixIndex = 0;
    double speed = static_cast<double>(tuplesPerSecond);

    while (speed >= 1000 && suffixIndex < (sizeof(suffixes) / sizeof(suffixes[0])) - 1) {
        speed /= 1000;
        ++suffixIndex;
    }

    std::stringstream stream;
    stream << COUT_FLOAT_(2) << speed << " " << suffixes[suffixIndex];

    return stream.str();
}

ALWAYS_INLINE std::string colorString(const std::string & text, const std::string& colorCode)
{
    return colorCode + text + "\033[0m";
}

auto high_resolution_time() { return std::chrono::high_resolution_clock::now(); }
auto elapsed_time_ns(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end) { return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); }
auto elapsed_time_us(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end) { return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(); }
auto elapsed_time_ms(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end) { return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); }

ALWAYS_INLINE uint64_t current_time_nsecs()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec)*1000000000L + t.tv_nsec;
}

ALWAYS_INLINE uint64_t current_time_usecs()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec) * 1000000L + t.tv_nsec;
}

template <typename T>
void print_performance(
    std::string name,
    size_t num_tuples,
    double time_diff_ns,
    std::string color = COLOR_WHITE
)
{
    size_t throughput_tps = (num_tuples / time_diff_ns) * 1e9;
    size_t throughput_bs = (num_tuples * sizeof(T) / time_diff_ns) * 1e9;

    std::cout
        << COUT_HEADER_SMALL << name << ": "
        << COUT_HEADER_SMALL << "\tTime: "       << COUT_FLOAT_(2) << time_diff_ns / 1e6 << " ms"
        << COUT_HEADER_SMALL << "\tThroughput: "
        << colorString(formatTuplesPerSecond(throughput_tps), color)
        << colorString(formatBytesPerSecond(throughput_bs), color)
        << '\n';
}

template <typename T>
void print_performance(
    std::string name,
    size_t num_tuples,
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end,
    std::string color = COLOR_WHITE
)
{
    auto time_diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    print_performance<T>(name, num_tuples, time_diff, color);
}

template <typename T>
size_t fill_batch_with_dataset(
    const std::vector<T, aligned_allocator<T>> & dataset,
    T * batch,
    const size_t size,
    const size_t offset = 0
)
{
    size_t total_copied = 0;

    size_t n = size / dataset.size();
    size_t remaining_elems = size % dataset.size();
    size_t remaining = dataset.size() - offset;

    T * bufferStart = batch;
    if (offset > 0 && n > 0) {

        if (remaining > size) {
            memcpy(bufferStart, dataset.data() + offset, size * sizeof(T));
            total_copied += size;
            return size;
        }

        memcpy(bufferStart, dataset.data() + offset, remaining * sizeof(T));
        bufferStart += remaining;

        total_copied += remaining;

        n = (size - remaining) / dataset.size();
        remaining_elems = (size - remaining) % dataset.size();
    }

    for (size_t i = 0; i < n; i++) {
        memcpy(bufferStart, dataset.data(), dataset.size() * sizeof(T));
        bufferStart += dataset.size();
        total_copied += dataset.size();
    }

    if (remaining_elems > 0) {
        memcpy(bufferStart, dataset.data(), remaining_elems * sizeof(T));
        total_copied += remaining_elems;
    }

    return total_copied;
}

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

} // namespace fx

#endif // __HOST_UTILS__
