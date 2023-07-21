#ifndef __DATASET_HPP__
#define __DATASET_HPP__

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "constants.hpp"
#include "record_t.hpp"


// information contained in each record in the dataset
typedef enum { DATE_FIELD, TIME_FIELD, EPOCH_FIELD, DEVICE_ID_FIELD, TEMP_FIELD, HUMID_FIELD, LIGHT_FIELD, VOLT_FIELD } record_field;
// fields that can be monitored by the user
typedef enum { TEMPERATURE, HUMIDITY, LIGHT, VOLTAGE } monitored_field;

// type of the input records: < date_value, time_value, epoch_value, device_id_value, temp_value, humid_value, light_value, voltage_value>
using sd_record_t = std::tuple<std::string, std::string, int, int, double, double, double, double>;

std::vector<sd_record_t> load_datasaet(const std::string & file_path)
{
    std::vector<sd_record_t> parsed_file;

    std::ifstream file(file_path);
    // check if the file exists
    if (!file.good()) {
        std::cout << "Error: file " << file_path << " does not exist" << std::endl;
        exit(-50);
        //return parsed_file;
    }

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            int token_count = 0;
            std::vector<std::string> tokens;

            std::istringstream iss(line);
            std::string item;
            while (iss >> item) {
                tokens.push_back(item);
                token_count++;
            }

            // a record is valid if it contains at least 8 values (one for each field of interest)
            if (token_count >= 8) {
                // save parsed file
                sd_record_t r(tokens.at(DATE_FIELD),
                           tokens.at(TIME_FIELD),
                           atoi(tokens.at(EPOCH_FIELD).c_str()),
                           atoi(tokens.at(DEVICE_ID_FIELD).c_str()),
                           atof(tokens.at(TEMP_FIELD).c_str()),
                           atof(tokens.at(HUMID_FIELD).c_str()),
                           atof(tokens.at(LIGHT_FIELD).c_str()),
                           atof(tokens.at(VOLT_FIELD).c_str()));
                parsed_file.push_back(r);
            }
        }
        file.close();
    }
    return parsed_file;
}

std::vector<record_t, fx::aligned_allocator<record_t>> generate_tuples_for_key(size_t key, size_t size)
{
    std::vector<record_t, fx::aligned_allocator<record_t>> dataset(size);
    for (size_t i = 0; i < size; ++i) {
        dataset[i].key = key;
        dataset[i].property_value = (i % WIN_SIZE == (WIN_SIZE / 2)) ? 1.12f : 1.0f;
    }
    return dataset;
}

std::vector<record_t, fx::aligned_allocator<record_t>> generate_dataset(size_t size, size_t idx)
{
    std::vector<record_t, fx::aligned_allocator<record_t>> dataset(size);
    size_t tuples_per_key = size / MAX_KEYS;
    if (tuples_per_key % TUPLES_PER_KEY != 0) {
        std::cout << "WARNING: size should be a multiple of " << TUPLES_PER_KEY << std::endl;
    }

    for (size_t i = 0; i < tuples_per_key; ++i) {
        for (size_t j = 0; j < MAX_KEYS; ++j) {
            dataset[i * MAX_KEYS + j].key = (j + idx) % MAX_KEYS;
            dataset[i * MAX_KEYS + j].property_value = (i % WIN_SIZE == (WIN_SIZE / 2)) ? 1.12f : 1.0f;
        }
    }
    return dataset;

    // std::vector< std::vector<record_t, fx::aligned_allocator<record_t>> > datasets;
    // datasets.reserve(MAX_KEYS);

    // size_t tuples_per_key = size / MAX_KEYS;
    // std::cout << "Generating dataset of size " << size << " (" << tuples_per_key << " tuples per key)" << std::endl;

    // if (tuples_per_key % TUPLES_PER_KEY != 0) {
    //     std::cout << "WARNING: size should be a multiple of " << TUPLES_PER_KEY << std::endl;
    // }

    // for (size_t i = 0; i < MAX_KEYS; ++i) {
    //     std::vector<record_t, fx::aligned_allocator<record_t> > dataset = generate_tuples_for_key((i + idx) % MAX_KEYS, tuples_per_key);
    //     datasets.push_back(dataset);
    // }

    // std::vector<record_t, fx::aligned_allocator<record_t> > dataset(MAX_KEYS * TUPLES_PER_KEY);
    // for (size_t i = 0; i < TUPLES_PER_KEY; ++i) {
    //     for (size_t j = 0; j < MAX_KEYS; ++j) {
    //         dataset[i * MAX_KEYS + j] = datasets[j][i];
    //     }
    // }

    // return dataset;
}

template <typename T>
std::vector<T, fx::aligned_allocator<T> > get_dataset(const std::string & dataset_filepath,
                           const monitored_field field)
{
    std::vector<sd_record_t> parsed_file = load_datasaet(dataset_filepath);
    std::vector<T, fx::aligned_allocator<T> > dataset;
    dataset.reserve(parsed_file.size());

    int timestamp = 0;
    for (const auto & record : parsed_file) {
        T t;
        t.key = std::get<DEVICE_ID_FIELD>(record);
        switch (field) {
            case TEMPERATURE: t.property_value = std::get<TEMP_FIELD>(record);  break;
            case HUMIDITY:    t.property_value = std::get<HUMID_FIELD>(record); break;
            case LIGHT:       t.property_value = std::get<LIGHT_FIELD>(record); break;
            case VOLTAGE:     t.property_value = std::get<VOLT_FIELD>(record);  break;
        }
        t.timestamp = timestamp++;
        dataset.push_back(t);
    }
    return dataset;
}

#endif // __DATASET_HPP__
