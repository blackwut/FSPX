#ifndef __TYPEHANDLER_HPP__
#define __TYPEHANDLER_HPP__

#include <ap_int.h>

// Code taken from hlslib:
// https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/DataPack.h#L27

template <typename T>
struct TypeHandler {
    static constexpr int WIDTH = 8 * sizeof(T);

    static T from_ap(ap_uint<WIDTH> const & ap) {
        return *reinterpret_cast<T const *>(&ap);
    }

    static ap_uint<WIDTH> to_ap(T const & value) {
        return *reinterpret_cast<ap_uint<WIDTH> const *>(&value);
    }
};

#endif // __TYPEHANDLER_HPP__
