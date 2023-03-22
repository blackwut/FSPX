#ifndef __CONNECTORS_HPP__
#define __CONNECTORS_HPP__


#include "stream.hpp"


namespace fx {


struct RoundRobin {};
struct LoadBalancer {};
struct KeyBy {};
struct Broadcast {};
// struct UserDefined {};


template <size_t N, typename T>
void StoSN(
    hls::stream<T> & istrm,
    hls::stream<bool> & e_istrm,
    hls::stream<T> & ostrms[N],
    hls::stream<bool> & e_ostrms[N],
    RoundRobin policy
)
{
    bool id = 0;
    bool last = e_istrm.read();

MAIN_LOOP_StoSN:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        
        ostrms[id].write(t);
        e_ostrms[id].write(false);

        last = e_istrm.read();
        id = (id + 1 == N) ? 0 : (id + 1);
    }
    
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        e_ostrms[i].write(true);
    }
}


template <size_t N, typename T>
void StoSN(
    hls::stream<T> & istrm,
    hls::stream<bool> & e_istrm,
    hls::stream<T> & ostrms[N],
    hls::stream<bool> & e_ostrms[N],
    LoadBalancer policy
)
{
    int id = 0;
    bool last = e_istrm.read();

MAIN_LOOP_StoSN:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        if (false == ostrms[id].full()) {
            T t = istrm.read();
        
            ostrms[id].write(t);
            e_ostrms[id].write(false);

            last = e_istrm.read();
        }
        id = (id + 1 == N) ? 0 : (id + 1);
    }
    
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        e_ostrms[i].write(true);
    }
}


template <size_t N, typename T, typename T_KEY_EXTRACTOR>
void StoSN(
    hls::stream<T> & istrm,
    hls::stream<bool> & e_istrm,
    hls::stream<T> & ostrms[N],
    hls::stream<bool> & e_ostrms[N],
    KeyBy policy,
    T_KEY_EXTRACTOR && key_extractor
)
{
    bool last = e_istrm.read();

MAIN_LOOP_StoSN:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        int key = key_extractor(t);
    
        ostrms[key].write(t);
        e_ostrms[key].write(false);

        last = e_istrm.read();
    }
    
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        e_ostrms[i].write(true);
    }
}


template <size_t N, typename T>
void StoSN(
    hls::stream<T> & istrm,
    hls::stream<bool> & e_istrm,
    hls::stream<T> & ostrms[N],
    hls::stream<bool> & e_ostrms[N],
    Broadcast policy
)
{
    bool last = e_istrm.read();

MAIN_LOOP_StoSN:
    while (!last) {
    #pragma HLS PIPELINE II = 1
    #pragma HLS LOOP_TRIPCOUNT min = 1 max = 1024
        T t = istrm.read();
        
        for (int i = 0; i < N; ++i) {
        #pragma HLS UNROLL
            ostrms[i].write(t);
            e_ostrms[id].write(false);
        }

        last = e_istrm.read();
    }
    
    for (int i = 0; i < N; ++i) {
    #pragma HLS UNROLL
        e_ostrms[i].write(true);
    }
}

}

#endif // __CONNECTORS_HPP__