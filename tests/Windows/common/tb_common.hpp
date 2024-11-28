#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <map>
#include <algorithm>
#include "fspx.hpp"

#include "tuples.hpp"

// define TEST_DEBUG if you want to print the input and output tuples for debugging

std::vector<tuple_t> generate_input_randomly(int n, int max_keys, int seed, int from = 1, int to = 16, int timestamp_start = 0)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(from, to);

    std::vector<tuple_t> data;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < max_keys; ++k) {
            tuple_t d;
            d.key = k;
            d.value = dist(gen);
            d.aggregate = 0;
            d.timestamp = timestamp_start + dist(gen);

            data.push_back(d);
        }
    }
    return data;
}

void write_input(in_stream_t & in, const std::vector<tuple_t> & data, bool eos = false)
{
    std::cout << "Writing input..." << std::endl;
    #if defined(TEST_DEBUG)
    std::cout << std::setw(8) << "key"       << ", "
              << std::setw(8) << "value"     << ", "
              << std::setw(8) << "aggregate" << ", "
              << std::setw(8) << "timestamp" << std::endl;
    #endif

    for (const auto & d : data) {
        in.write(d);

        #if defined(TEST_DEBUG)
        std::cout << std::setw(8) << d.key       << ", "
                  << std::setw(8) << d.value     << ", "
                  << std::setw(8) << d.aggregate << ", "
                  << std::setw(8) << d.timestamp << std::endl;
        #endif
    }

    if (eos) {
        in.write_eos();
    }
}

std::vector<output_t> read_output(out_stream_t & out)
{
    std::cout << "Reading output..." << std::endl;

    #if defined(TEST_DEBUG)
    std::cout << std::setw(8) << "i"        << ", "
              << std::setw(8) << "key"      << ", "
              << std::setw(8) << "val"      << ", "
              << std::setw(8) << "agg"      << ", "
              << std::setw(8) << "count/ts" << std::endl;
    unsigned int i = 0;
    #endif

    std::vector<output_t> result;

    bool last = out.read_eos();
    while (!last) {
        output_t r = out.read();
        result.push_back(r);
        last = out.read_eos();

        #if defined(TEST_DEBUG)
        std::cout << std::setw(8) << i++         << ", "
                  << std::setw(8) << r.key       << ", "
                  << std::setw(8) << r.wid       << ", "
                  << std::setw(8) << r.value     << ", "
                  << std::setw(8) << r.timestamp << std::endl;
        #endif
    }

    return result;
}

// std::vector<output_t> read_output(out_stream_t & out)
// {
//     std::cout << "Reading output..." << std::endl;

//     #if defined(TEST_DEBUG)
//     std::cout << std::setw(8) << "i"         << ", "
//               << std::setw(8) << "key"       << ", "
//               << std::setw(8) << "val"       << ", "
//               << std::setw(8) << "agg"       << ", "
//               << std::setw(8) << "timestamp" << std::endl;
//     unsigned int i = 0;
//     #endif

//     std::vector<output_t> result;
//     std::map<unsigned int, unsigned int> last_timestamp;

//     bool last = out.read_eos();
//     while (!last) {
//         output_t r = out.read();
//         result.push_back(r);
//         last = out.read_eos();

//         if (last_timestamp.find(r.key) == last_timestamp.end()) {
//             last_timestamp[r.key] = r.timestamp;
//         } else {
//             if (r.timestamp < last_timestamp[r.key]) {
//                 std::cout << "ERROR: key " << r.key << " has timestamp " << r.timestamp << " that is less than the last timestamp " << last_timestamp[r.key] << std::endl;
//             }
//             last_timestamp[r.key] = r.timestamp;
//         }

//         #if defined(TEST_DEBUG)
//         std::cout << std::setw(8) << i++         << ", "
//                   << std::setw(8) << r.key       << ", "
//                   << std::setw(8) << r.wid       << ", "
//                   << std::setw(8) << r.value     << ", "
//                   << std::setw(8) << r.timestamp << std::endl;
//         #endif
//     }

//     return result;
// }

bool check_results(const std::vector<output_t> data, const std::vector<output_t> expected, bool ordered = true)
{
    bool success = true;
    if (data.size() != expected.size()) {
        std::cout << "ERROR: expected " << expected.size() << " elements, but got " << data.size() << std::endl;

        std::cout << "Expected elements: " << std::endl;
        for (const output_t e : expected) {
            std::cout << e << std::endl;
        }

        success = false;
        return success;
    }

    // make a copy of expected
    std::vector<output_t> expected_copy = expected;

    if (data.size() == 0) {
        return success;
    }

    if (ordered) {
        for (unsigned int i = 0; i < data.size(); ++i) {
            if (data[i].key != expected[i].key || data[i].wid != expected[i].wid || data[i].value != expected[i].value || data[i].timestamp != expected[i].timestamp) {
                std::cout << "ERROR: result "
                          << data[i]
                          << " != expected "
                          << expected[i]
                          << std::endl;
                success = false;
                break;
            }
        }
    } else {
        for (const output_t d : data) {
            auto it = std::find_if(expected_copy.begin(), expected_copy.end(), [&d](const output_t& e) {
                return e.key == d.key && e.wid == d.wid && e.value == d.value && e.timestamp == d.timestamp;
            });
            if (it == expected_copy.end()) {
                std::cout << "ERROR: element " << d << " not found in expected results" << std::endl;
                success = false;
                break;
            } else {
                expected_copy.erase(it);
            }
        }
    }

    return success;
}

void test(std::vector<tuple_t> input_data, std::vector<output_t> expected_output, std::string test_name = "", bool ordered = true)
{
    std::cout << "Running test: " << test_name << std::endl;
    in_stream_t in("in");
    out_stream_t out("out");

    write_input(in, input_data, true);
    kernel(in, out);
    auto output = read_output(out);
    bool success = check_results(output, expected_output, ordered);
    if (success) {
        std::cout << "Test " << test_name << " PASSED" << std::endl;
    } else {
        std::cout << "Test " << test_name << " FAILED" << std::endl;
        exit(1);
    }
}
