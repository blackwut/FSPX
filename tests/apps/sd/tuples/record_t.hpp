#ifndef __RECORD_T_HPP__
#define __RECORD_T_HPP__

#include "host/utils.hpp"

struct record_t
{
    unsigned int key;
    float property_value;
    float incremental_average;
    unsigned int timestamp;

    record_t() = default;

#if !defined(__SYNTHESIS__)
    bool operator==(const record_t& rhs) const
    {
        return (key == rhs.key)
            && fx::approximatelyEqual(property_value, rhs.property_value)
            && fx::approximatelyEqual(incremental_average, rhs.incremental_average)
            && (timestamp == rhs.timestamp);
    }

    bool operator!=(const record_t& rhs) const
    {
        return !operator==(rhs);
    }
#endif
};

#endif // __RECORD_T_HPP__