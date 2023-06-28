#include "kernel.hpp"
#include "../common/sd_dataset.hpp"
#include <algorithm>

int main() {

    // load dataset
    std::vector<record_t> dataset = get_dataset<record_t>("dataset.dat", TEMPERATURE);
    std::cout << dataset.size() << " tuples loaded!" << std::endl;

    axis_stream_t in[SO_PAR];
    axis_stream_t out[SI_PAR];

    for (int i = 0; i < SO_PAR; ++i){
        for (int j = 0; j < SIZE; ++j) {
            // record_t r = new_record_spike(j, 4);
            record_t r = dataset[i * SO_PAR + j];
            in[i].write(r);
        }   
    }

    for (int i = 0; i < SO_PAR; ++i) {
        in[i].write_eos();
    }

    test(in, out);

    // MovingAverage<float> ma{};
    // SpikeDetector sd{};

    // int i = 0;
    // bool last = false;
    // record_t r = out.read(last);
    // while (!last) {
    //     record_t r_test = new_record(i);
    //     record_t r_ma = ma(r_test);
    //     record_t r_sd;
    //     bool res = sd(r_ma, r_sd);
    //     if (res) {
    //         // print_record(r_sd);
    //         // print_record(r);
    //         if (r != r_sd) {
    //             return 1;
    //         }
    //         r = out.read(last);
    //     }
    //     ++i;
    // }


    // collect results from output streams
    std::vector <record_t> results;
    for (int i = 0; i < SI_PAR; ++i) {
        bool last = out[i].read_eos();
        while (!last) {
            record_t r = out[i].read();
            last = out[i].read_eos();
            results.push_back(r);
        }
    }




    // for (int i = 0; i < SI_PAR; ++i) {
    //     int j = 0;
    //     bool last = out[i].read_eos();
    //     while (!last) {
    //         record_t r = out[i].read();
    //         last = out[i].read_eos();

    //         if (r != golden_results[j]) {
    //             std::cout << "ERROR: " << std::endl;
    //             print_record(r);
    //             print_record(golden_results[j]);
    //             return 1;
    //         }
    //         j++;
    //     }
    // }

    std::vector<record_t> golden_results = get_golden_results();

    auto sorter = [](record_t a, record_t b) {
        if (a.key < b.key) return true;
        if ((a.key == b.key) && (a.property_value < b.property_value)) return true;
        if ((a.key == b.key) && (a.property_value == b.property_value) && (a.incremental_average < b.incremental_average)) return true;
        if ((a.key == b.key) && (a.property_value == b.property_value) && (a.incremental_average == b.incremental_average) && (a.timestamp < b.timestamp)) return true;
        return false;
    };

    // compare the results
    std::sort(results.begin(), results.end(), sorter);
    std::sort(golden_results.begin(), golden_results.end(), sorter);

    std::cout << "results size: " << results.size() << std::endl;
    std::cout << "golden results size: " << golden_results.size() << std::endl;

    // print golden and results
    for (int i = 0; i < results.size(); ++i) {
        std::cout << "R(" << i << "): "; print_record(results[i]);
    }

    for (int i = 0; i < results.size(); ++i) {
        std::cout << "G(" << i << "): "; print_record(results[i]);
    }


    if (results.size() != golden_results.size()) {
        std::cout << "ERROR: results size mismatch!" << std::endl;
        return 1;
    }

    for (int i = 0; i < results.size(); ++i) {
        if (results[i] != golden_results[i]) {
            std::cout << "ERROR: results mismatch!" << std::endl;
            print_record(results[i]);
            print_record(golden_results[i]);
            return 1;
        }
    }

    

    return 0;
}
