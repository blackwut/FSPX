#ifndef __TUPLE_HPP__
#define __TUPLE_HPP__

struct record_t
{
    static constexpr int MAX_KEY_VALUE = 64;

    unsigned int key;
    unsigned int val;

    record_t() = default;

    #ifndef __SYNTHESIS__
    record_t(unsigned int key, float val)
    : key(key)
    , val(val)
    {
    }
    #endif // __SYNTHESIS__

    bool operator==(const record_t& rhs) const
    {
        return (key == rhs.key) && (val == rhs.val);
    }

    bool operator!=(const record_t& rhs) const
    {
        return !operator==(rhs);
    }
};

#endif // __TUPLE_HPP__