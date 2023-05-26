#ifndef __DATAPACK_HPP__
#define __DATAPACK_HPP__

#include <ap_int.h>

// Code taken from hlslib:
// https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/DataPack.h#L113

namespace fx
{

template <typename T, int N>
class DataPackProxy; // Forward declaration

template <typename T, int N>
class DataPack
{
    static_assert(N > 0, "N must be positive");

public:

    static constexpr int WIDTH = TypeHandler<T>::WIDTH;
    using item_t = ap_uint<WIDTH>;
    using items_t = ap_uint<N * WIDTH>;

    // Default constructor
    DataPack() : items_() {}

    // Copy constructor for ap_uint<N * WIDTH>
    DataPack(items_t const & items) : items_(items) {}

    // Copy constructor for DataPack
    DataPack(DataPack<T, N> const & other) = default;

    // Move constructor for DataPack
    DataPack(DataPack<T, N> && other) = default;

    // Copy constructor for T
    DataPack(T const & value) : items_()
    {
    #pragma HLS INLINE
        fill(value);
    }

    // Copy constructor for T[N]
    // explicit DataPack(T const arr[N])
    // : items_()
    // {
    // #pragma HLS INLINE
    //     pack(arr);
    // }

    DataPack<T, N>& operator=(DataPack<T, N> && other)
    {
    #pragma HLS INLINE
        items_ = other.items_;
        return *this;
    }

    // Assignment operator for DataPack
    DataPack<T, N>& operator=(DataPack<T, N> const &other)
    {
    #pragma HLS INLINE
        items_ = other.items_;
        return *this;
    }

    T get(int i) const
    {
    #pragma HLS INLINE
    // #ifndef __SYNTHESIS__
    //     if (i < 0 || i >= N) {
    //         std::stringstream ss;
    //         ss << "Index " << i << " out of range for DataPack of " << N << " items";
    //         throw std::out_of_range(ss.str());
    //     }
    // #endif
        item_t temp = items_.range((i + 1) * WIDTH - 1, i * WIDTH);
        return TypeHandler<T>::from_ap(temp);
    }

    void set(int i, T value)
    {
    #pragma HLS INLINE
    // #ifndef __SYNTHESIS__
    //     if (i < 0 || i >= N) {
    //         std::stringstream ss;
    //         ss << "Index " << i << " out of range for DataPack of " << N << " items";
    //         throw std::out_of_range(ss.str());
    //     }
    // #endif
        items_.range((i + 1) * WIDTH - 1, i * WIDTH) = (
            TypeHandler<T>::to_ap(value)
        );
    }

    void set(int i, item_t value)
    {
    #pragma HLS INLINE
    // #ifndef __SYNTHESIS__
    //     if (i < 0 || i >= N) {
    //         std::stringstream ss;
    //         ss << "Index " << i << " out of range for DataPack of " << N << " items";
    //         throw std::out_of_range(ss.str());
    //     }
    // #endif
        items_.range((i + 1) * WIDTH - 1, i * WIDTH) = value;
    }

    void fill(T const & value)
    {
    #pragma HLS INLINE
    DataPack_fill:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            set(i, value);
        }
    }

    void pack(T const arr[N])
    {
    #pragma HLS INLINE
    DataPack_pack:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            set(i, arr[i]);
        }
    }

    void unpack(T arr[N]) const
    {
    #pragma HLS INLINE
    DataPack_Unpack:
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            arr[i] = get(i);
        }
    }

    T operator[](const int i) const {
    #pragma HLS INLINE
        return get(i);
    }

    item_t operator()(const int i) {
    #pragma HLS INLINE
        return items_.range((i + 1) * WIDTH - 1, i * WIDTH);
    }

    // implicit conversion
    operator items_t() const {
    #pragma HLS INLINE
        return items_;
    }

    DataPackProxy<T, N> operator[](const int i);

  // Access to internal data directly if necessary
  items_t & data() { return items_; }
  items_t data() const { return items_; }

private:

  items_t items_;
};


template <typename T, int N>
class DataPackProxy
{
    static constexpr int WIDTH = TypeHandler<T>::WIDTH;
    using item_t = ap_uint<WIDTH>;

public:

    DataPackProxy(DataPack<T, N> & data, int index)
    : index_(index), data_(data)
    {
        #pragma HLS INLINE
    }

    DataPackProxy(DataPackProxy<T, N> const &) = default;

    DataPackProxy(DataPackProxy<T, N> &&) = default;

    ~DataPackProxy() {}

    void operator=(T const & rhs) {
    #pragma HLS INLINE
        data_.set(index_, rhs);
    }

    void operator=(T && rhs) {
    #pragma HLS INLINE
        data_.set(index_, rhs);
    }

    void operator=(item_t const & rhs) {
    #pragma HLS INLINE
        data_.set(index_, rhs);
    }

    void operator=(item_t && rhs) {
    #pragma HLS INLINE
        data_.set(index_, rhs);
    }

    // void operator=(DataPackProxy<T, N> const & rhs) {
    // #pragma HLS INLINE
    //     data_.set(index_, static_cast<T>(rhs));
    // }

    // void operator=(DataPackProxy<T, N> &&rhs) {
    // #pragma HLS INLINE
    //     data_.set(index_, static_cast<T>(rhs));
    // }

    operator T() const {
    #pragma HLS INLINE
        return data_.get(index_);
    }

    operator item_t() const {
    #pragma HLS INLINE
        return data_.get(index_);
    }

 private:

  int index_;
  DataPack<T, N> & data_;

};

template <typename T, int N>
DataPackProxy<T, N> DataPack<T, N>::operator[](const int i) {
    #pragma HLS INLINE
    return DataPackProxy<T, N>(*this, i);
}

} // namespace fx

#endif // __DATAPACK_HPP__
