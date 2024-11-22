#ifndef __STREAMS_HPP__
#define __STREAMS_HPP__

#include "stream.hpp"
#include "axis.hpp"

// namespace fx {

// template <typename T, unsigned int N, unsigned int DEPTH = 2>
// struct streams
// {
//     using stream_t = stream<T, DEPTH>;
//     using data_t = T;

//     stream_t streams[N];
    
//     T read(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].read();
//     }

//     void write(unsigned int i, const T & v)
//     {
//     #pragma HLS INLINE
//         streams[i].write(v);
//     }

//     bool read_eos(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].read_eos();
//     }

//     void write_eos(unsigned int i)
//     {
//     #pragma HLS INLINE
//         streams[i].write_eos();
//     }

//     bool empty(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].empty();
//     }

//     bool empty_eos(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].empty_eos();
//     }

//     bool full(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].full();
//     }

//     bool full_eos(unsigned int i)
//     {
//     #pragma HLS INLINE
//         return streams[i].full_eos();
//     }

//     void write_all(const T & v)
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             streams[i].write(v);
//         }
//     }

//     void write_eos_all()
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             streams[i].write_eos();
//         }
//     }

//     bool empty_all()
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             if (!streams[i].empty()) {
//                 return false;
//             }
//         }
//         return true;
//     }

//     bool empty_eos_all()
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             if (!streams[i].empty_eos()) {
//                 return false;
//             }
//         }
//         return true;
//     }

//     bool full_all()
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             if (!streams[i].full()) {
//                 return false;
//             }
//         }
//         return true;
//     }

//     bool full_eos_all()
//     {
//     #pragma HLS INLINE
//         for (unsigned int i = 0; i < N; ++i) {
//         #pragma HLS UNROLL
//             if (!streams[i].full_eos()) {
//                 return false;
//             }
//         }
//         return true;
//     }
// };

// } // namespace fx

#endif // __STREAMS_HPP__
