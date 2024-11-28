#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "fspx.hpp"
#include "tuples.hpp"
#include "window_functors.hpp"

namespace WindowTest {

struct STuple_t
{
    size_t seq;
    tuple_t tuple;

    STuple_t(size_t seq, tuple_t tuple)
        : seq(seq), tuple(tuple)
    {}
};

using Tuples_t = std::vector<STuple_t>;
using KeyedTuples_t = std::unordered_map<size_t, Tuples_t>;


void print_vector(std::vector<tuple_t> & v) {
    for (tuple_t & t : v) {
        std::cout << t << std::endl;
    }
}

void print_map(KeyedTuples_t map) {
    for (auto & pair : map) {
        const size_t key = pair.first;
        Tuples_t & tuples = pair.second;
        
        std::cout << "Key: " << key << std::endl;
        for (STuple_t & st : tuples) {
            const size_t seq = st.seq;
            const tuple_t & t = st.tuple;
            std::cout << "  Sequence: " << seq << " - tuple: " << t << std::endl;
        }
    }
}

KeyedTuples_t ignore_keys(std::vector<tuple_t> & inputs)
{
    KeyedTuples_t map;

    for (size_t i = 0; i < inputs.size(); i++) {
        tuple_t t = inputs[i];

        if (map.count(0) == 0) {
            // Create a new array for this key
            Tuples_t bucket;
            bucket.emplace_back(i, t);
            map[0] = bucket;
        } else {
            // Add to the bucket
            map[0].emplace_back(i, t);
        }
    }
    return map;
}

KeyedTuples_t groupby_key(std::vector<tuple_t> & inputs)
{
    KeyedTuples_t map;

    for (size_t i = 0; i < inputs.size(); i++) {
        tuple_t t = inputs[i];

        if (map.count(t.key) == 0) {
            // Create a new array for this key
            Tuples_t bucket;
            bucket.emplace_back(i, t);
            map[t.key] = bucket;
        } else {
            // Add to the bucket
            map[t.key].emplace_back(i, t);
        }
    }
    return map;
}


KeyedTuples_t drop_outdated_tuples(KeyedTuples_t & map, const size_t lateness)
{
    KeyedTuples_t new_map;

    for (auto & pair : map) {
        const size_t key = pair.first;
        Tuples_t & tuples = pair.second;

        Tuples_t new_tuples;
        fx::timestamp_t max_timestamp = lateness;

        for (STuple_t & st : tuples) {
            const size_t seq = st.seq;
            const tuple_t & t = st.tuple;

            bool drop = (t.timestamp < max_timestamp - lateness);
            if (!drop) {
                new_tuples.push_back(STuple_t{seq, t});
                max_timestamp = std::max(max_timestamp, t.timestamp);
            }
        }
        new_map[key] = new_tuples;
    }

    return new_map;
}

template <typename functor_t>
STuple_t apply_functor(Tuples_t & inputs, functor_t functor, const size_t wid, const size_t key, const bool is_count = false)
{
    result_t result;

    size_t max_seq = 0;
    fx::timestamp_t max_timestamp = 0;
    for (auto & st : inputs) {
        const size_t seq = st.seq;
        const tuple_t & t = st.tuple;
        functor(t, result);

        max_seq = std::max(max_seq, seq);
        max_timestamp = std::max(max_timestamp, t.timestamp);
    }

    tuple_t output;
    output.key = key;
    output.value = wid;
    output.aggregate = result.result();
    output.timestamp = (is_count ? inputs.size() : max_timestamp);

    return STuple_t{max_seq, output};
}

template <typename functor_t>
std::vector<output_t> keyed_sliding_window(KeyedTuples_t & map, functor_t functor, size_t size, size_t step, const bool is_count = false)
{
    std::vector<STuple_t> outputs;

    // for each key
    for (auto & pair : map) {
        const size_t key = pair.first;
        Tuples_t & tuples = pair.second;

        size_t wid = 0;
        for (size_t i = 0; i < tuples.size(); i += step) {
            auto begin = tuples.begin() + i;
            auto end = begin + size;
            if (end > tuples.end()) {
                end = tuples.end();
            }

            Tuples_t sub_tuples(begin, end);
            STuple_t result = apply_functor(sub_tuples, functor, wid, key, is_count);
            outputs.push_back(result);

            wid++;
        }
    }

    std::sort(outputs.begin(), outputs.end(), [](const STuple_t & a, const STuple_t & b) {
        return a.seq < b.seq;
    });

    std::vector<output_t> outputs_tuples;
    for (auto & st : outputs) {
        outputs_tuples.push_back(output_t{fx::key_t(st.tuple.key), fx::wid_t(st.tuple.value), st.tuple.aggregate, fx::timestamp_t(st.tuple.timestamp)});
    }

    return outputs_tuples;
}

// TumblingCountWindow
template <size_t SIZE, typename functor_t> 
std::vector<output_t> get_results_TumblingCountWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = ignore_keys(inputs);
    return keyed_sliding_window(grouped_tuples, functor, SIZE, SIZE, true);
}

// SlidingCountWindow
template <size_t SIZE, size_t STEP, typename functor_t>
std::vector<output_t> get_results_SlidingCountWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = ignore_keys(inputs);
    return keyed_sliding_window(grouped_tuples, functor, SIZE, STEP, true);
}

// KeyedTumblingCountWindow
template <size_t SIZE, typename functor_t>
std::vector<output_t> get_results_KeyedTumblingCountWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = groupby_key(inputs);
    return keyed_sliding_window(grouped_tuples, functor, SIZE, SIZE, true);
}

// KeyedSlidingCountWindow
template <size_t SIZE, size_t STEP, typename functor_t>
std::vector<output_t> get_results_KeyedSlidingCountWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = groupby_key(inputs);
    return keyed_sliding_window(grouped_tuples, functor, SIZE, STEP, true);
}


// TumblingTimeWindow
template <size_t SIZE, size_t LATENESS, typename functor_t>
std::vector<output_t> get_results_TumblingTimeWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = ignore_keys(inputs);
    auto remaining_tuples = drop_outdated_tuples(grouped_tuples, LATENESS);
    return keyed_sliding_window(remaining_tuples, functor, SIZE, SIZE);
}

// SlidingTimeWindow
template <size_t SIZE, size_t STEP, size_t LATENESS, typename functor_t>
std::vector<output_t> get_results_SlidingTimeWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = ignore_keys(inputs);
    auto remaining_tuples = drop_outdated_tuples(grouped_tuples, LATENESS);
    return keyed_sliding_window(remaining_tuples, functor, SIZE, STEP);
}

// KeyedTumblingTimeWindow
template <size_t SIZE, size_t LATENESS, typename functor_t>
std::vector<output_t> get_results_KeyedTumblingTimeWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = groupby_key(inputs);
    auto remaining_tuples = drop_outdated_tuples(grouped_tuples, LATENESS);
    return keyed_sliding_window(remaining_tuples, functor, SIZE, SIZE);
}

// KeyedSlidingTimeWindow
template <size_t SIZE, size_t STEP, size_t LATENESS, typename functor_t>
std::vector<output_t> get_results_KeyedSlidingTimeWindow(std::vector<tuple_t> & inputs, functor_t functor)
{
    auto grouped_tuples = groupby_key(inputs);
    auto remaining_tuples = drop_outdated_tuples(grouped_tuples, LATENESS);
    return keyed_sliding_window(remaining_tuples, functor, SIZE, STEP);
}

} // namespace WindowTest
