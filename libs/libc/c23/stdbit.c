/*
 * C23 <stdbit.h> implementation.
 *
 * One STDBIT_OPS block per unsigned type generates all 14 operations from the
 * compiler bit builtins. Narrow types are zero-extended into the promotion
 * type (UP) and the leading-zero count is corrected by (bits(UP) - W).
 */

#include <stdbit.h>

#define STDBIT_OPS(SUF, T, W, UP, CLZ, CTZ, POP)                              \
    unsigned int stdc_leading_zeros_##SUF(T v)                                \
    {                                                                         \
        return v ? (unsigned int)CLZ((UP)v) -                                 \
                       ((unsigned int)(sizeof(UP) * 8) - (unsigned int)(W))   \
                 : (unsigned int)(W);                                         \
    }                                                                         \
    unsigned int stdc_trailing_zeros_##SUF(T v)                               \
    {                                                                         \
        return v ? (unsigned int)CTZ((UP)v) : (unsigned int)(W);              \
    }                                                                         \
    unsigned int stdc_count_ones_##SUF(T v)                                   \
    {                                                                         \
        return (unsigned int)POP((UP)v);                                      \
    }                                                                         \
    unsigned int stdc_count_zeros_##SUF(T v)                                  \
    {                                                                         \
        return (unsigned int)(W) - (unsigned int)POP((UP)v);                  \
    }                                                                         \
    unsigned int stdc_leading_ones_##SUF(T v)                                 \
    {                                                                         \
        return stdc_leading_zeros_##SUF((T)~v);                               \
    }                                                                         \
    unsigned int stdc_trailing_ones_##SUF(T v)                                \
    {                                                                         \
        return stdc_trailing_zeros_##SUF((T)~v);                              \
    }                                                                         \
    unsigned int stdc_first_leading_zero_##SUF(T v)                           \
    {                                                                         \
        unsigned int lo = stdc_leading_ones_##SUF(v);                         \
        return lo == (unsigned int)(W) ? 0u : lo + 1u;                        \
    }                                                                         \
    unsigned int stdc_first_leading_one_##SUF(T v)                            \
    {                                                                         \
        unsigned int lz = stdc_leading_zeros_##SUF(v);                        \
        return lz == (unsigned int)(W) ? 0u : lz + 1u;                        \
    }                                                                         \
    unsigned int stdc_first_trailing_zero_##SUF(T v)                          \
    {                                                                         \
        unsigned int to = stdc_trailing_ones_##SUF(v);                        \
        return to == (unsigned int)(W) ? 0u : to + 1u;                        \
    }                                                                         \
    unsigned int stdc_first_trailing_one_##SUF(T v)                           \
    {                                                                         \
        unsigned int tz = stdc_trailing_zeros_##SUF(v);                       \
        return tz == (unsigned int)(W) ? 0u : tz + 1u;                        \
    }                                                                         \
    _Bool stdc_has_single_bit_##SUF(T v)                                      \
    {                                                                         \
        return (_Bool)(POP((UP)v) == 1);                                      \
    }                                                                         \
    unsigned int stdc_bit_width_##SUF(T v)                                    \
    {                                                                         \
        return (unsigned int)(W) - stdc_leading_zeros_##SUF(v);               \
    }                                                                         \
    T stdc_bit_floor_##SUF(T v)                                               \
    {                                                                         \
        return v ? (T)((T)1 << (stdc_bit_width_##SUF(v) - 1u)) : (T)0;         \
    }                                                                         \
    T stdc_bit_ceil_##SUF(T v)                                                \
    {                                                                         \
        return (v <= 1) ? (T)1                                                \
                        : (T)((T)1 << stdc_bit_width_##SUF((T)(v - 1)));       \
    }

STDBIT_OPS(uc, unsigned char, 8, unsigned int,
           __builtin_clz, __builtin_ctz, __builtin_popcount)
STDBIT_OPS(us, unsigned short, 16, unsigned int,
           __builtin_clz, __builtin_ctz, __builtin_popcount)
STDBIT_OPS(ui, unsigned int, 32, unsigned int,
           __builtin_clz, __builtin_ctz, __builtin_popcount)
STDBIT_OPS(ul, unsigned long, 32, unsigned long,
           __builtin_clzl, __builtin_ctzl, __builtin_popcountl)
STDBIT_OPS(ull, unsigned long long, 64, unsigned long long,
           __builtin_clzll, __builtin_ctzll, __builtin_popcountll)
