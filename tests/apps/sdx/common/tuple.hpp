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

    #ifndef __SYNTHESIS__
    record_t(unsigned int key, float property_value, float incremental_average, unsigned int timestamp)
    : key(key)
    , property_value(property_value)
    , incremental_average(incremental_average)
    , timestamp(timestamp)
    {
    }
    #endif // __SYNTHESIS__

    bool operator==(const record_t& rhs) const
    {
        return (key == rhs.key)
            && approximatelyEqual(property_value, rhs.property_value, 0.00001f)
            && approximatelyEqual(incremental_average, rhs.incremental_average, 0.00001f)
            // && (timestamp == rhs.timestamp)
            ;
    }

    bool operator!=(const record_t& rhs) const
    {
        return !operator==(rhs);
    }
};

#endif // __TUPLE_HPP__