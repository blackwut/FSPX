#include "kernel.hpp"

bool test_s_to_s()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out, TestCase::StoS);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        int r = out.read();
        last = out.read_eos();

        if (i != r) {
            return false;
        }
        ++i;
    }

    return true;
}

bool test_round_robin()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out, TestCase::RoundRobin);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        int r = out.read();
        last = out.read_eos();

        if (i != r) {
            return false;
        }
        ++i;
    }

    return true;
}

bool test_load_balancer()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out, TestCase::LoadBalancer);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        int r = out.read();
        last = out.read_eos();

        if (i != r) {
            return false;
        }
        ++i;
    }

    return true;
}

bool test_key_by()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out, TestCase::KeyBy);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        int r = out.read();
        last = out.read_eos();

        if (i != r) {
            return false;
        }
        ++i;
    }

    return true;
}

bool test_broadcast()
{
    stream_t in;
    stream_t out;

    for (int i = 0; i < SIZE; ++i) {
        in.write(i);
    }
    in.write_eos();

    test(in, out, TestCase::Broadcast);

    int i = 0;
    bool last = out.read_eos();
    while (!last) {
        for (int n = 0; n < N; ++n) {
            int r = out.read();
            last = out.read_eos();

            if (i != r) {
                return false;
            }
        }
        ++i;
    }

    return true;
}

// bool test_s_to_s()
// {
//     stream_t in;
//     stream_t out;

//     for (int i = 0; i < SIZE; ++i) {
//         in.write(i);
//     }
//     in.write_eos();

//     test(in, out);

//     int count = 0;
//     int i = 0;
//     bool last = out.read_eos();
//     while (!last) {
//         for (int n = 0; n < N; ++n) {
//             int r = out.read();
//             last = out.read_eos();

//             // std::cout << "received: " << r << " vs " << (i + n) << std::endl;
//             if ((i + n) != r) {
//                 return false;
//             }
//         }

//         count++;
//         if (count % N == 0) {
//             i += 4;
//         }
//     }

//     return true;
// }

int test_stream_connectors(int type)
{
    bool success = true;

    switch (type) {
        case 0: { success = test_s_to_s();          if (!success) return 1; } break;
        case 1: { success = test_round_robin();     if (!success) return 2; } break;
        case 2: { success = test_load_balancer();   if (!success) return 3; } break;
        case 3: { success = test_key_by();          if (!success) return 4; } break;
        case 4: { success = test_broadcast();       if (!success) return 5; } break;
        default:
            return 6;
    }

    return 0;
}


int main(int argc, char * argv[])
{
    argc--;
    argv++;

    int test_case = atoi(argv[0]);

    int err = test_stream_connectors(test_case);
    return err;
}
