#include "kernel.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <map>
#include <algorithm>

#define _DEBUG 1


// multiple keys
// random timestamps single key
// random timestamps multiple keys

// limit cases:
// - empty input

std::vector<data_t> generate_input_single_key(int n)
{
    std::vector<data_t> data;
    for (int i = 0; i < n / 2; ++i) {
        data.push_back(data_t(0, 0, 1, i));
    }

    for (int i = 0; i < n / 2; ++i) {
        data.push_back(data_t(0, 0, 1, i));
    }
    return data;
}

std::vector<data_t> generate_input_multiple_keys(int n, int max_keys)
{
    std::vector<data_t> data;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < max_keys; ++k) {
            data.push_back(data_t(k, 0, 1, i));
        }
    }

    return data;
}

std::vector<data_t> generate_input_random(int n, int max_keys, int seed)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(- (WINDOW_LATENESS * 2), WINDOW_LATENESS * 2);

    std::vector<data_t> data;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < max_keys; ++k) {
            auto random_value = dist(gen);
            data_t d;
            d.key = k;
            d.value = i;
            d.aggregate = 0;
            d.timestamp = i + (i + random_value > 0 ? random_value : 0);

            data.push_back(d);
        }
    }
    return data;
}


void write_input(in_stream_t & in, const std::vector<data_t> & data, bool eos = false)
{
    std::cout << "Writing input..." << std::endl;
    #if _DEBUG
    std::cout << std::setw(8) << "key"       << ", "
              << std::setw(8) << "value"     << ", "
              << std::setw(8) << "aggregate" << ", "
              << std::setw(8) << "timestamp" << std::endl;
    #endif

    for (const auto & d : data) {
        in.write(d);

        #if _DEBUG
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

std::vector<data_t> read_output(out_stream_t & out)
{
    std::cout << "Reading output..." << std::endl;

    #if _DEBUG
    std::cout << std::setw(8) << "i"         << ", "
              << std::setw(8) << "key"       << ", "
              << std::setw(8) << "val"       << ", "
              << std::setw(8) << "agg"       << ", "
              << std::setw(8) << "timestamp" << std::endl;
    #endif

    std::vector<data_t> result;
    std::map<unsigned int, unsigned int> last_timestamp;

    unsigned int i = 0;
    bool last = out.read_eos();
    while (!last) {
        data_t r = out.read();
        result.push_back(r);
        last = out.read_eos();

        if (last_timestamp.find(r.key) == last_timestamp.end()) {
            last_timestamp[r.key] = r.timestamp;
        } else {
            if (r.timestamp < last_timestamp[r.key]) {
                std::cerr << "Error: key " << r.key << " has timestamp " << r.timestamp << " that is less than the last timestamp " << last_timestamp[r.key] << std::endl;
            }
            last_timestamp[r.key] = r.timestamp;
        }

        #if _DEBUG
        std::cout << std::setw(8) << i++         << ", "
                  << std::setw(8) << r.key       << ", "
                  << std::setw(8) << r.value     << ", "
                  << std::setw(8) << r.aggregate << ", "
                  << std::setw(8) << r.timestamp << std::endl;
        #endif
    }

    return result;
}


bool check_results(const std::vector<data_t> data, const std::vector<data_t> expected)
{
    bool success = true;
    if (data.size() != expected.size()) {
        std::cerr << "Error: expected " << expected.size() << " elements, but got " << data.size() << std::endl;
        success = false;
    }

    // make a copy of expected
    std::vector<data_t> expected_copy = expected;

    if (data.size() == 0) {
        return success;
    }

    // check if data[i] is present in expected and remove it from expected
    for (const data_t d : data) {
        auto it = std::find_if(expected_copy.begin(), expected_copy.end(), [&d](const data_t& e) {
            return e.key == d.key && e.aggregate == d.aggregate && e.timestamp == d.timestamp; // ignoring .value in the comparison
        });
        if (it == expected_copy.end()) {
            std::cerr << "Error: element with key " << d.key << " not found in expected results" << std::endl;
            success = false;
        }
        expected_copy.erase(it);
    }

    return success;
}

void test(std::vector<data_t> input_data, std::vector<data_t> expected_output, std::string test_name = "")
{
    std::cout << "Running test: " << test_name << std::endl;
    in_stream_t in("in");
    out_stream_t out("out");

    write_input(in, input_data, true);
    kernel(in, out);
    bool success = check_results(read_output(out), expected_output);
    if (success) {
        std::cout << "Test " << test_name << " passed" << std::endl;
    } else {
        std::cerr << "Test " << test_name << " failed" << std::endl;
        exit(1);
    }
}

int main() {

    // empty input
    std::vector<data_t> test_input_empty = {};
    std::vector<data_t> test_output_empty = {};
    test(test_input_empty, test_output_empty, "empty");

    // single key
    std::vector<data_t> test_input_single_key = generate_input_single_key(20);
    std::vector<data_t> test_output_single_key = {
        {0, 0, 6, 0},
        {0, 0, 9, 3},
        {0, 0, 8, 6},
        {0, 0, 2, 9}
    };

    test(test_input_single_key, test_output_single_key, "single_key");

    // multiple keys
    std::vector<data_t> test_input_multiple_keys = generate_input_multiple_keys(20, 2);
    std::vector<data_t> test_output_multiple_keys = {
        {0, 0, 5,  0},
        {0, 0, 5,  3},
        {0, 0, 5,  6},
        {0, 0, 5,  9},
        {0, 0, 5, 12},
        {0, 0, 5, 15},
        {0, 0, 2, 18},
        {1, 0, 5,  0},
        {1, 0, 5,  3},
        {1, 0, 5,  6},
        {1, 0, 5,  9},
        {1, 0, 5, 12},
        {1, 0, 5, 15},
        {1, 0, 2, 18}
    };
    test(test_input_multiple_keys, test_output_multiple_keys, "multiple_keys");

    // test random single key
    std::vector<data_t> test_input_random_single_key = generate_input_random(20, 1, 42);
    std::vector<data_t> test_output_random_single_key = {
        {0, 0, 1,  0},
        {0, 0, 1,  7},
        {0, 0, 5,  7},
        {0, 0, 5, 11},
        {0, 0, 1, 12},
        {0, 0, 1, 18},
        {0, 0, 4, 22},
        {0, 0, 3, 22}
    };
    test(test_input_random_single_key, test_output_random_single_key, "random_single_key");

    // test random multiple keys
    std::vector<data_t> test_input_random_multiple_keys = generate_input_random(10, MAX_KEYS, 42);
    std::vector<data_t> test_output_random_multiple_keys = {
        {0, 0, 3,  0},
        {1, 0, 2,  2},
        {1, 0, 4,  6},
        {0, 0, 3,  6},
        {0, 0, 4,  6},
        {0, 0, 1, 13},
        {0, 0, 1, 13},
        {1, 0, 2,  6},
        {1, 0, 2, 15},
        {1, 0, 1, 15},
        {2, 0, 2,  9},
        {2, 0, 2,  9},
        {2, 0, 1, 15},
        {2, 0, 1, 15},
        {3, 0, 4,  0},
        {3, 0, 4,  3},
        {3, 0, 6,  7},
        {3, 0, 3, 10}
    };
    test(test_input_random_multiple_keys, test_output_random_multiple_keys, "random_multiple_keys");

    return 0;
}
