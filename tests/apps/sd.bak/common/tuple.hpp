#ifndef __TUPLE_HPP__
#define __TUPLE_HPP__

struct record_t
{
    static constexpr int MAX_KEY_VALUE = 64;

    unsigned int key;
    float property_value;
    float incremental_average;
    unsigned int timestamp;

    record_t() = default;

    bool operator==(const record_t& rhs) const
    {
        return (key == rhs.key)
            && approximatelyEqual(property_value, rhs.property_value)
            && approximatelyEqual(incremental_average, rhs.incremental_average)
            && (timestamp == rhs.timestamp);
    }

    bool operator!=(const record_t& rhs) const
    {
        return !operator==(rhs);
    }
};

#endif // __TUPLE_HPP__