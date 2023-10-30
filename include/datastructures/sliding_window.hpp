#ifndef __SLIDING_WINDOW_HPP__
#define __SLIDING_WINDOW_HPP__

// # of buckets = ceil(SIZE / STEP)
// shift window by STEP

#define ENABLE_IMPLEMENTATION 3 // 0: good, 1: same as 0 , 2: high resources


namespace fx {

template <
    typename OPERATOR,  // functor to combine two elements
    unsigned int SIZE,  // window size
    unsigned int STEP   // step size
>
struct SlidingCountWindow {
    // number of buckets
    static constexpr unsigned int BUCKETS = DIV_CEIL(SIZE, STEP);

    using IN_T  = typename OPERATOR::IN_T;
    using AGG_T = typename OPERATOR::AGG_T;
    using OUT_T = typename OPERATOR::OUT_T;

    AGG_T buckets[BUCKETS];
    ap_uint<BUCKETS> init;
    unsigned int count;

    SlidingCountWindow()
    : init(0)
    , count(0)
    {
        #pragma HLS ARRAY_PARTITION variable = buckets complete dim = 1
        #pragma HLS ARRAY_PARTITION variable = init complete dim = 0
    }

#if ENABLE_IMPLEMENTATION == 0
    // II = 1 depth = 7
    void update(const IN_T & in, OUT_T & out, bool & valid_out)
    {
    #pragma HLS INLINE

        AGG_T _out = OPERATOR::identity();
        const bool enable_shift = (count == (SIZE - 1));

        BUCKETING:
        for (int i = 0; i < BUCKETS; ++i) {
        #pragma HLS UNROLL
            AGG_T tmp = buckets[i];
            const AGG_T val = (init[i] ? tmp : OPERATOR::identity());
            init[i] = 1;

            const bool update_bucket = (i * STEP <= count && count < (i * STEP + SIZE));
            const AGG_T agg = (update_bucket ? OPERATOR::combine(OPERATOR::lift(in), val) : val);
            buckets[i] = agg;
        }

        if (enable_shift) {
            _out = buckets[0];
        SHIFT_BUCKETS:
            for (int i = 0; i < BUCKETS - 1; ++i) {
            #pragma HLS UNROLL
                    buckets[i] = buckets[i + 1];
            }
            buckets[BUCKETS - 1] = OPERATOR::identity();
        }

        if (enable_shift) {
            out = OPERATOR::lower(_out);
            valid_out = true;
            count -= (STEP - 1);
        } else {
            out = OPERATOR::lower(OPERATOR::identity());
            valid_out = false;
            count++;
        }
    }
#elif ENABLE_IMPLEMENTATION == 1
    // II = 1 depth = 7
    void update(const IN_T & in, OUT_T & out, bool & valid_out)
    {
    #pragma HLS INLINE

        AGG_T _out = OPERATOR::identity();
        const bool enable_shift = (count == (SIZE - 1));

        BUCKETING:
        for (int i = 0; i < BUCKETS; ++i) {
        #pragma HLS UNROLL
            AGG_T tmp = buckets[i];
            const AGG_T val = (init[i] ? tmp : OPERATOR::identity());
            init[i] = 1;

            const bool update_bucket = (i * STEP <= count && count < (i * STEP + SIZE));
            AGG_T agg = (update_bucket ? OPERATOR::combine(OPERATOR::lift(in), val) : val);

            if (enable_shift) {
                if (i == 0) {
                    _out = agg;
                } else {
                    buckets[i - 1] = agg;
                }
            } else {
                buckets[i] = agg;
            }
        }

        if (enable_shift) {
            buckets[BUCKETS - 1] = OPERATOR::identity();

            out = OPERATOR::lower(_out);
            valid_out = true;

            count -= (STEP - 1);
        } else {
            out = OPERATOR::lower(OPERATOR::identity());
            valid_out = false;

            count++;
        }
    }
#elif ENABLE_IMPLEMENTATION == 2
    // ------------ WARNING -----------
    // TODO: CONSUMES A LOT OF RESOURCES!!!
    // TODO: reports shows a long critical path on INIT (I guess!!)
    // II = 1 depth = 7
    void update(const IN_T & in, OUT_T & out, bool & valid)
    {
    // #pragma HLS INLINE

    BUCKETING:
        for (int i = 0; i < BUCKETS; ++i) {
        #pragma HLS UNROLL
            if (i * STEP <= count && count < (i * STEP + SIZE)) {
                const AGG_T tmp = (init[i] ? buckets[i] : OPERATOR::identity());
                buckets[i] = OPERATOR::combine(OPERATOR::lift(in), tmp);
                init[i] = 1;
            }
        }

        if (count == (SIZE - 1)) {
            out = OPERATOR::lower(buckets[0]);
            valid = true;
            count -= (STEP - 1);

        SW_SHIFT:
            for (int i = 0; i < BUCKETS - 1; ++i) {
                buckets[i] = buckets[i + 1];
            }
            buckets[BUCKETS - 1] = OPERATOR::identity();

        } else {
            out = OPERATOR::identity();
            valid = false;
            count++;
        }

    //     const bool enable_shift = (count == (SIZE - 1));
    //     const AGG_T agg = (enable_shift ? buckets[0] : OPERATOR::identity());
    // SW_SHIFT:
    //     for (int i = 0; i < BUCKETS - 1; ++i) {
    //         if (enable_shift) {
    //             buckets[i] = buckets[i + 1];
    //         }
    //     }
    //     if (enable_shift) buckets[BUCKETS - 1] = OPERATOR::identity();

    //     out = OPERATOR::lower(agg);
    //     valid = enable_shift;

    //     if (enable_shift) {
    //         count -= (STEP - 1);
    //     } else {
    //         count++;
    //     }
    }
#elif ENABLE_IMPLEMENTATION == 3
// II = 1 depth = 7
    void update(const IN_T & in, OUT_T & out, bool & valid_out)
    {
    // #pragma HLS INLINE

        const bool enable_shift = (count == (SIZE - 1));
        const AGG_T lifted_in = OPERATOR::lift(in);

        BUCKETING:
        for (int i = 0; i < BUCKETS; ++i) {
        #pragma HLS UNROLL
            const bool update_bucket = (i * STEP <= count && count < (i * STEP + SIZE));
            AGG_T tmp = buckets[i];
            buckets[i] = (update_bucket ? OPERATOR::combine(lifted_in, tmp) : lifted_in);
        }

        const AGG_T _out = (enable_shift ? buckets[0] : OPERATOR::identity());

        if (enable_shift) {
        SHIFT_BUCKETS:
            for (int i = 0; i < BUCKETS - 1; ++i) {
            #pragma HLS UNROLL
                    buckets[i] = buckets[i + 1];
            }
            buckets[BUCKETS - 1] = OPERATOR::identity();
        }

        if (enable_shift) {
            out = OPERATOR::lower(_out);
            valid_out = true;
            count -= (STEP - 1);
        } else {
            out = OPERATOR::lower(OPERATOR::identity());
            valid_out = false;
            count++;
        }
    }
#endif
};

template <
    typename OPERATOR,  // functor to combine two elements
    unsigned int SIZE,  // window size
    unsigned int STEP,  // step size
    unsigned int KEYS   // max number of keys
>
struct KeyedSlidingCountWindow {

    using IN_T  = typename OPERATOR::IN_T;
    using OUT_T = typename OPERATOR::OUT_T;

    SlidingCountWindow<OPERATOR, SIZE, STEP> windows[KEYS];

    KeyedSlidingCountWindow()
    {
        #pragma HLS ARRAY_PARTITION variable = windows complete dim = 1
    }

    void update(const unsigned int & key, const IN_T & in, OUT_T & out, bool & valid)
    {
    #pragma HLS INLINE
        windows[key].update(in, out, valid);
    }
};

} // namespace fx

#endif // __SLIDING_WINDOW_HPP__