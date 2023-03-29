#include "kernel.hpp"

void s_to_s(
    stream_t & in,
    stream_t & out
)
{
    fx::StoS(in, out);
}

void round_robin(
    stream_t & in,
    stream_t & out
)
{
    fx::RoundRobin policy;
    stream_t internal_streams[N];
#pragma HLS DATAFLOW
    fx::StoSN<N>(in, internal_streams, policy);
    fx::SNtoS<N>(internal_streams, out, policy);
}

void load_balancer(
    stream_t & in,
    stream_t & out
)
{
    fx::LoadBalancer policy;
    stream_t internal_streams[N];
#pragma HLS DATAFLOW
    fx::StoSN<N>(in, internal_streams, policy);
    fx::SNtoS<N>(internal_streams, out, policy);
}

void key_by(
    stream_t & in,
    stream_t & out
)
{
    fx::KeyBy policy;
    stream_t internal_streams[N];
#pragma HLS DATAFLOW
    fx::StoSN<N>(in, internal_streams, policy,
        [](data_t & t){
            return (t % N);
        });
    fx::SNtoS<N>(internal_streams, out, policy,
        [](int index){
        #pragma HLS INLINE
            return index % N;
        });
}

void broadcast(
    stream_t & in,
    stream_t & out
)
{
    fx::Broadcast b_policy;
    fx::RoundRobin rr_policy;
    stream_t internal_streams[N];
#pragma HLS DATAFLOW
    fx::StoSN<N>(in, internal_streams, b_policy);
    fx::SNtoS<N>(internal_streams, out, rr_policy);
}

void test(
    stream_t & in,
    stream_t & out,
    TestCase t
)
{
    switch (t) {
        case TestCase::StoS:         s_to_s(in, out);          break;
        case TestCase::RoundRobin:   round_robin(in, out);     break;
        case TestCase::LoadBalancer: load_balancer(in, out);   break;
        case TestCase::KeyBy:        key_by(in, out);          break;
        case TestCase::Broadcast:    broadcast(in, out);       break;
    }
}
