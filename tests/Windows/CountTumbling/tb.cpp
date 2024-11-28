#include "kernel.hpp"

#define TEST_DEBUG

#define WINDOW_FUNCTOR_COUNT
#include "tb_common.hpp"
#include "window_test.hpp"

int main()
{
    std::vector<tuple_t> input_empty = {};
    std::vector<output_t> output_empty = WindowTest::get_results_TumblingCountWindow<WINDOW_SIZE>(input_empty, window_functor());
    test(input_empty, output_empty, "empty");

    std::vector<tuple_t> input_random = generate_input_randomly(128, 1, 16);
    std::vector<output_t> output_random = WindowTest::get_results_TumblingCountWindow<WINDOW_SIZE>(input_random, window_functor());
    test(input_random, output_random, "random");
}
