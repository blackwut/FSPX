#include <iostream>
#include <iomanip>

#include "fspx.hpp"
#include "tuples.hpp"
#include "window_functors.hpp"

//*****************************************************************************
//
// Configuration
//
//*****************************************************************************

static constexpr unsigned int WINDOW_SIZE = 16;
static constexpr unsigned int WINDOW_STEP = 3;
static constexpr unsigned int WINDOW_LATENESS = 2;
static constexpr unsigned int MAX_KEY = 4;

#if defined(WINDOW_FUNCTOR_COUNT)
    using result_t = result_count_t;
    using window_functor = window_functor_count;
#elif defined(WINDOW_FUNCTOR_MEAN)
    using result_t = result_mean_t;
    using window_functor = window_functor_mean;
#else
    #error "Please define WINDOW_FUNCTOR_SUM or WINDOW_FUNCTOR_MEAN"
#endif


//*****************************************************************************
//
// Kernel stuff
//
//*****************************************************************************

using in_stream_t = fx::axis_stream<tuple_t, 64>;
using out_stream_t = fx::axis_stream<output_t, 64>;

void kernel(
    in_stream_t & in,
    out_stream_t & out
);

struct Drainer
{
    template <typename window_result_t>
    void operator()(const window_result_t & in, output_t & out) {
    #pragma HLS INLINE

        const result_t result = in.unwrap();

        out.key = in.get_key();
        out.wid = in.get_wid();
        out.value = result.result();
        out.timestamp = in.get_count();
    }
};