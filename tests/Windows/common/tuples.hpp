#ifndef __TUPLES_HPP__
#define __TUPLES_HPP__


//*****************************************************************************
//
// Tuple definition
//
//*****************************************************************************

struct tuple_t
{
    unsigned int key;
    float value;
    float aggregate;
    fx::timestamp_t timestamp;

    tuple_t() = default;

    tuple_t(unsigned int key, float value, float aggregate, unsigned int timestamp)
        : key(key), value(value), aggregate(aggregate), timestamp(timestamp)
    {}

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const tuple_t & d)
    {
        os << "("
           << "key: "       << std::setw(3) << d.key       << ", "
           << "value: "     << std::setw(3) << d.value     << ", " 
           << "aggregate: " << std::setw(3) << d.aggregate << ", "
           << "timestamp: " << std::setw(3) << d.timestamp
           << ")";
        return os;
    }
    #endif
};


auto tuple_key_extractor = [](const tuple_t & t) { return t.key; };


struct result_count_t
{
    unsigned int count;

    result_count_t()
    : count(0)
    {}

    unsigned int key() const {
        return 0;
    }

    unsigned int result() const {
        return count;
    }

    unsigned int timestamp() const {
        return 0;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const result_count_t & d)
    {
        os << "(count: " << std::setw(3) << d.count << ")";
        return os;
    }
    #endif
};

struct result_mean_t
{
    float sum;
    unsigned int count;

    result_mean_t()
    : sum(0)
    , count(0)
    {}

    unsigned int key() const {
        return 0;
    }

    float result() const {
        return sum / count;
    }

    unsigned int timestamp() const {
        return 0;
    }

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const result_mean_t & d)
    {
        os << "("
           << "sum: "   << std::setw(3) << d.sum   << ", "
           << "count: " << std::setw(3) << d.count
           << ")";
        return os;
    }
    #endif
};


struct output_t
{
    fx::key_t key;
    fx::wid_t wid;
    float value;
    fx::timestamp_t timestamp;

    #if !defined(__SYNTHESIS__)
    friend std::ostream & operator<<(std::ostream & os, const output_t & d)
    {
        os << "("
           << "key: "      << std::setw(3) << d.key       << ", "
           << "wid: "      << std::setw(3) << d.wid       << ", "
           << "value: "    << std::setw(3) << d.value     << ", "
           << "timestamp: "<< std::setw(3) << d.timestamp
           << ")";
        return os;
    }
    #endif
};

#endif // __TUPLES_HPP__
