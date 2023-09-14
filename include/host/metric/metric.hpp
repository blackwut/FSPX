#ifndef __METRIC_METRIC_HPP__
#define __METRIC_METRIC_HPP__

#include <cstdint>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>


namespace fx {

class Metric {

private:

    std::string name_;
    std::vector<double> samples_;
    uint64_t total_;

public:

    Metric(const std::string &name)
    : name_(name)
    {}

    void add(double value) { samples_.push_back(value); }

    void total(uint64_t total) { total_ = total; }

    uint64_t total() { return total_; }

    uint64_t getN() { return samples_.size(); }

    double mean()
    {
        return std::accumulate(samples_.begin(), samples_.end(), 0.0) / samples_.size();
    }

    double min()
    {
        return *std::min_element(samples_.begin(), samples_.end());
    }

    double max()
    {
        return *std::max_element(samples_.begin(), samples_.end());
    }

    double percentile(double percentile)
    {
        auto pointer = samples_.begin() + samples_.size() * percentile;
        std::nth_element(samples_.begin(), pointer, samples_.end());
        return *pointer;
    }
};

}

#endif // __METRIC_METRIC_HPP__
