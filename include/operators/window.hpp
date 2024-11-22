#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include "../common.hpp"
#include "../streams/streams.hpp"
#include "../datastructures/count_window.hpp"
#include "../datastructures/time_window.hpp"

namespace fx {

//******************************************************************************
//
// Count Windows
//
//******************************************************************************

template <
    typename window_functor_t,
    unsigned int SIZE,
    typename stream_in_t,
    typename stream_out_t
>
void CountTumblingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out
)
{
    using window_t = fx::Window::CountTumbling<window_functor_t, SIZE>;
    using wrapper_t = typename window_t::wrapper_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");

    #pragma HLS DATAFLOW
    
    fx::WindowForwardFlush<wrapper_t>(stream_in, wrapper_stream);
    fx::Filter<window_t>(wrapper_stream, stream_out);
}

template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t
>
void KeyedCountTumblingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    key_extractor_t && key_extractor
)
{
    using window_t = fx::Window::KeyedCountTumbling<window_functor_t, MAX_KEY, SIZE>;
    using wrapper_t = typename window_t::wrapper_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");

    #pragma HLS DATAFLOW
    
    KeyedWindowForwardFlush<wrapper_t, MAX_KEY>(
        stream_in, wrapper_stream, std::forward<key_extractor_t>(key_extractor)
    );
    Filter<window_t>(wrapper_stream, stream_out);
}

template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int STEP,
    typename stream_in_t,
    typename stream_out_t
>
void CountSlidingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out
)
{
    using window_t = fx::Window::CountSliding<window_functor_t, SIZE, STEP>;
    using wrapper_t = typename window_t::wrapper_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");

    #pragma HLS DATAFLOW
    
    WindowForwardFlush<wrapper_t>(stream_in, wrapper_stream);
    Filter<window_t>(wrapper_stream, stream_out);
}

template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE,
    unsigned int STEP,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t
>
void KeyedCountSlidingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    key_extractor_t && key_extractor
)
{
    using window_t = fx::Window::KeyedCountSliding<window_functor_t, MAX_KEY, SIZE, STEP>;
    using wrapper_t = typename window_t::wrapper_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");

    #pragma HLS DATAFLOW
    
    KeyedWindowForwardFlush<wrapper_t, MAX_KEY>(
        stream_in, wrapper_stream, std::forward<key_extractor_t>(key_extractor)
    );
    Filter<window_t>(wrapper_stream, stream_out);
}


//******************************************************************************
//
// Time Windows
//
//******************************************************************************

template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int LATENESS,
    typename stream_in_t,
    typename stream_out_t
>
void TimeTumblingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out
)
{
    using window_t = fx::Window::TimeTumbling<window_functor_t, SIZE, LATENESS>;
    using wrapper_t = typename window_t::wrapper_t;
    using wrapper_result_t = typename window_t::wrapper_result_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");
    fx::stream<wrapper_result_t, 64> result_streams[window_t::N];

    #pragma HLS DATAFLOW
    
    fx::WindowForwardFlush<wrapper_t>(stream_in, wrapper_stream);
    fx::ParallelFlatMap<window_t, window_t::N>(wrapper_stream, result_streams);
    fx::route_min_rec<window_t::N>(result_streams, stream_out,
        [](const auto & a, const auto & b) {
            return a.get_wid() < b.get_wid();
        }
    );
}

template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int LATENESS,
    unsigned int MAX_KEY,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t
>
void KeyedTimeTumblingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    key_extractor_t && key_extractor
)
{
    using window_t = fx::Window::KeyedTimeTumbling<window_functor_t, MAX_KEY, SIZE, LATENESS>;
    using wrapper_t = typename window_t::wrapper_t;
    using wrapper_result_t = typename window_t::wrapper_result_t;

    fx::stream<wrapper_t> wrapper_stream("wrapper_stream");
    fx::stream<wrapper_result_t> result_strms[window_t::N];

    #pragma HLS DATAFLOW
    
    fx::KeyedWindowForwardFlush<wrapper_t, MAX_KEY>(
        stream_in, wrapper_stream, std::forward<key_extractor_t>(key_extractor)
    );
    fx::ParallelFlatMap<window_t>(wrapper_stream, result_strms);
    fx::route_min_rec<window_t::N>(result_strms, stream_out,
        [](const auto & a, const auto & b) {
            if (a.get_sequence() != b.get_sequence()) {
                return a.get_sequence() < b.get_sequence();
            }
            return a.get_wid() < b.get_wid();
        }
    );
}

template <
    typename window_functor_t,
    unsigned int SIZE,
    unsigned int STEP,
    unsigned int LATENESS,
    typename stream_in_t,
    typename stream_out_t
>
void TimeSlidingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out
)
{
    using window_t = fx::Window::TimeSliding<window_functor_t, SIZE, STEP, LATENESS>;
    using wrapper_t = typename window_t::wrapper_t;
    using wrapper_result_t = typename window_t::wrapper_result_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");
    fx::stream<wrapper_result_t> result_strms[window_t::N];

    #pragma HLS DATAFLOW
    
    fx::WindowForwardFlush<wrapper_t>(stream_in, wrapper_stream);
    fx::ParallelFlatMap<window_t, window_t::N>(wrapper_stream, result_strms);
    fx::route_min_rec<window_t::N>(result_strms, stream_out,
        [](const auto & a, const auto & b) {
            if (a.get_sequence() != b.get_sequence()) {
                return a.get_sequence() < b.get_sequence();
            }
            return a.get_wid() < b.get_wid();
        }
    );
}

template <
    typename window_functor_t,
    unsigned int MAX_KEY,
    unsigned int SIZE,
    unsigned int STEP,
    unsigned int LATENESS,
    typename stream_in_t,
    typename stream_out_t,
    typename key_extractor_t
>
void KeyedTimeSlidingWindow (
    stream_in_t & stream_in,
    stream_out_t & stream_out,
    key_extractor_t && key_extractor
)
{
    using window_t = fx::Window::KeyedTimeSliding<window_functor_t, MAX_KEY, SIZE, STEP, LATENESS>;
    using wrapper_t = typename window_t::wrapper_t;
    using wrapper_result_t = typename window_t::wrapper_result_t;

    fx::stream<wrapper_t, 64> wrapper_stream("wrapper_stream");
    fx::stream<wrapper_result_t> result_strms[window_t::N];

    #pragma HLS DATAFLOW
    
    fx::KeyedWindowForwardFlush<wrapper_t, MAX_KEY>(
        stream_in, wrapper_stream, std::forward<key_extractor_t>(key_extractor)
    );
    fx::ParallelFlatMap<window_t, window_t::N>(wrapper_stream, result_strms);
    fx::route_min_rec<window_t::N>(result_strms, stream_out,
        [](const auto & a, const auto & b) {
            if (a.get_sequence() != b.get_sequence()) {
                return a.get_sequence() < b.get_sequence();
            }
            return a.get_wid() < b.get_wid();
        }
    );
}

} // namespace fx

#endif // __WINDOW_HPP__
