#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>


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

template <typename T>
std::vector<T> get_dataset(const std::string & dataset_filepath,
                           const monitored_field field)
{
    std::vector<sd_record_t> parsed_file = load_datasaet(dataset_filepath);
    std::vector<T> dataset;
    dataset.reserve(parsed_file.size());

    for (const auto & record : parsed_file) {
        T t;
        t.key = std::get<DEVICE_ID_FIELD>(record);
        switch (field) {
            case TEMPERATURE: t.property_value = std::get<TEMP_FIELD>(record);  break;
            case HUMIDITY:    t.property_value = std::get<HUMID_FIELD>(record); break;
            case LIGHT:       t.property_value = std::get<LIGHT_FIELD>(record); break;
            case VOLTAGE:     t.property_value = std::get<VOLT_FIELD>(record);  break;
        }
        dataset.push_back(t);
    }
    return dataset;
}