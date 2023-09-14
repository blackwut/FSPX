#ifndef __METRIC_SAMPLER_HPP__
#define __METRIC_SAMPLER_HPP__

#include <cstdint>
#include <vector>
#include <sys/time.h>
#include "../utils.hpp"

namespace fx {

class Sampler {

private:

    const uint64_t samples_per_second_;
    uint64_t epoch_;
    uint64_t counter_;
    uint64_t total_;
    std::vector<double> samples_;

public:

    Sampler(uint64_t samples_per_second)
    : samples_per_second_(samples_per_second)
    , epoch_(fx::current_time_nsecs())
    , counter_(0)
    , total_(0)
    {}

    void add(double value, uint64_t timestamp)
    {
        ++total_;

        // add samples according to the sample rate
        auto seconds = (timestamp - epoch_) / 1e9;
        if (samples_per_second_ == 0 || counter_ <= samples_per_second_ * seconds) {
            samples_.push_back(value);
            ++counter_;
        }
    }

    const std::vector<double> & values() const { return samples_; }

    uint64_t total() const { return total_; }

};

}

#endif // __METRIC_SAMPLER_HPP__
