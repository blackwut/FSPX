#include "kernel.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <map>

#define _DEBUG 1


std::vector<data_t> generate_test_even_timestamps(int n, int max_keys)
{
    std::vector<data_t> data;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < max_keys; ++k) {
            data_t d;
            d.key = k;
            d.value = i;
            d.aggregate = 0;
            d.timestamp = i * 2;

            data.push_back(d);
        }
    }
    return data;
}

std::vector<data_t> generate_test_random_timestamps(int n, int max_keys)
{
    // random generator with a given seed
    // std::mt19937 gen(42);

    // random generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 16);

    std::vector<data_t> data;
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < max_keys; ++k) {
            data_t d;
            d.key = k;
            d.value = i;
            d.aggregate = 0;
            d.timestamp = dist(gen);

            data.push_back(d);
        }
    }
    return data;
}

std::vector<data_t> generate_test_sorting_timestamp(int n, int max_key)
{
    // test with WINDOW_SIZE = 1 and WINDOW_LATENESS = 4
    std::vector<data_t> data;

    data.push_back(data_t(1, 0, 0, 3));
    // data.push_back(data_t(1, 0, 0, 2));
    data.push_back(data_t(1, 0, 0, 1));
    data.push_back(data_t(1, 0, 0, 0));
    data.push_back(data_t(0, 0, 0, 3));
    data.push_back(data_t(0, 0, 0, 2));
    // data.push_back(data_t(0, 0, 0, 1));
    data.push_back(data_t(0, 0, 0, 0));
    data.push_back(data_t(0, 0, 0, 9));
    data.push_back(data_t(1, 0, 0, 9));

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

// #if 0
//     // std::vector<unsigned int> keys = {
//     //     4, 3, 2, 1,
//     //     4, 3, 2, 1,
//     //     8, 7, 6, 5,
//     //     8, 7, 6, 5,
//     //     12, 11, 10, 9,
//     //     12, 11, 10, 9,
//     //     16, 15, 14, 13,
//     //     16, 15, 14, 13,
//     //     12, 10, 9, 11,
//     //     12, 10, 18, 11    
//     // };

//     std::vector<unsigned int> keys = {
//          0,  1,  2,  3,  4,  5,  6,  7,  8,
//          0,  1,  2,  3,  4,  5,  6,  7,  8,
//          9,  0,  1,  2,  3, 12, 10,  9, 11,
//         15,  0, 15, 15, 15, 15, 15, 15, 15,
//         15,  0, 15, 15, 15, 15, 15, 15, 15,
//         15, 15, 15, 15, 15, 15, 15, 15, 15
//     };

//     std::vector<unsigned int> timestamps = {
//          0,  1,  2,  3,  4,  5,  6,  7,  8,
//          0,  1,  2,  3,  4,  5,  6,  7,  8,
//          9,  21,  1,  2,  3, 12, 10,  9, 11,
//         15,  0, 15, 15, 15, 15, 15, 15, 15,
//         15,  0, 15, 15, 15, 15, 15, 15, 15,
//         15, 15, 15, 15, 15, 15, 15, 15, 15
//     };

//     // TODO: verifica che la tupla con chiave 0 e timestamp 21 vada in output il suo bucket

//     for (unsigned int i = 0; i < DATA_SIZE; ++i) {
//         data_t d;
//         d.key = keys[i % keys.size()]; // i;// % MAX_KEYS;
//         d.value = keys[i % keys.size()]; // i;
//         d.aggregate = 0;
//         d.timestamp = timestamps[i % timestamps.size()]; // i / MAX_KEYS;
//         in.write(d);

//         #if _DEBUG
//         std::cout << std::setw(8) << d.key       << ", "
//                   << std::setw(8) << d.value     << ", "
//                   << std::setw(8) << d.aggregate << ", "
//                   << std::setw(8) << d.timestamp << std::endl;
//         #endif
//     }
// #else
//     for (int i = 0; i < DATA_SIZE; ++i) {
//         data_t d;
//         d.key = i % 4;
//         d.value = i;
//         d.aggregate = 0;
//         d.timestamp = i / 4;

//         in.write(d);

//         #if _DEBUG
//         std::cout << std::setw(8) << d.key       << ", "
//                   << std::setw(8) << d.value     << ", "
//                   << std::setw(8) << d.aggregate << ", "
//                   << std::setw(8) << d.timestamp << std::endl;
//         #endif
//     }
// #endif

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

void read_output(out_stream_t & out)
{
    std::cout << "Reading output..." << std::endl;

    #if _DEBUG
    std::cout << std::setw(8) << "i"         << ", "
              << std::setw(8) << "key"       << ", "
              << std::setw(8) << "val"       << ", "
              << std::setw(8) << "agg"       << ", "
              << std::setw(8) << "timestamp" << std::endl;
    #endif

    std::map<unsigned int, unsigned int> last_timestamp;

    unsigned int i = 0;
    bool last = out.read_eos();
    while (!last) {
        data_t r = out.read();
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
}

void execute(in_stream_t & in, out_stream_t & out, bool eos = false)
{
    // std::vector<data_t> data = generate_test_even_timestamps(4, MAX_KEYS);
    std::vector<data_t> data = generate_test_random_timestamps(4, MAX_KEYS);
    // std::vector<data_t> data = generate_test_sorting_timestamp(4, MAX_KEYS);

    write_input(in, data, eos);
    test(in, out);
    read_output(out);
}

int main() {
    in_stream_t in("in");
    out_stream_t out("out");
    execute(in, out, true);


    // fx::stream<int, 2> in;
    // fx::stream<int, 2> out;

    // for (int i = 0; i < 10; ++i) {
    //     in.write(i);
    // }
    // in.write_eos();


    // test(in, out);

    // bool last = out.read_eos();
    // while (!last) {
    //     if (!out.empty() && !out.empty_eos()) {
    //         std::cout << out.read() << std::endl;
    //         last = out.read_eos();
    //     }
    // }

    return 0;
}
