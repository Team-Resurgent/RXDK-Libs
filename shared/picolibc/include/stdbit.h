/*
 * <stdbit.h> - C23 bit utilities (ISO/IEC 9899:2023 §7.18).
 *
 * Full implementation for RXDK-Libs: 14 operations over the five unsigned
 * integer types plus the type-generic macros. Definitions live in
 * libs/libc/c23/stdbit.c (built on the compiler bit builtins).
 */
#ifndef _STDBIT_H
#define _STDBIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __STDC_VERSION_STDBIT_H__ 202311L

#define __STDC_ENDIAN_LITTLE__ 1234
#define __STDC_ENDIAN_BIG__    4321
#define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_LITTLE__

/* Per-type prototypes. */
#define __STDBIT_DECL(SUF, T)                       \
    unsigned int stdc_leading_zeros_##SUF(T);       \
    unsigned int stdc_leading_ones_##SUF(T);        \
    unsigned int stdc_trailing_zeros_##SUF(T);      \
    unsigned int stdc_trailing_ones_##SUF(T);       \
    unsigned int stdc_first_leading_zero_##SUF(T);  \
    unsigned int stdc_first_leading_one_##SUF(T);   \
    unsigned int stdc_first_trailing_zero_##SUF(T); \
    unsigned int stdc_first_trailing_one_##SUF(T);  \
    unsigned int stdc_count_zeros_##SUF(T);         \
    unsigned int stdc_count_ones_##SUF(T);          \
    _Bool stdc_has_single_bit_##SUF(T);             \
    unsigned int stdc_bit_width_##SUF(T);           \
    T stdc_bit_floor_##SUF(T);                      \
    T stdc_bit_ceil_##SUF(T);

__STDBIT_DECL(uc, unsigned char)
__STDBIT_DECL(us, unsigned short)
__STDBIT_DECL(ui, unsigned int)
__STDBIT_DECL(ul, unsigned long)
__STDBIT_DECL(ull, unsigned long long)

#undef __STDBIT_DECL

/* Type-generic macros. */
#define __STDBIT_GENERIC(OP, x) _Generic((x),  \
    unsigned char: OP##_uc,                    \
    unsigned short: OP##_us,                   \
    unsigned int: OP##_ui,                     \
    unsigned long: OP##_ul,                    \
    unsigned long long: OP##_ull)(x)

#define stdc_leading_zeros(x)       __STDBIT_GENERIC(stdc_leading_zeros, (x))
#define stdc_leading_ones(x)        __STDBIT_GENERIC(stdc_leading_ones, (x))
#define stdc_trailing_zeros(x)      __STDBIT_GENERIC(stdc_trailing_zeros, (x))
#define stdc_trailing_ones(x)       __STDBIT_GENERIC(stdc_trailing_ones, (x))
#define stdc_first_leading_zero(x)  __STDBIT_GENERIC(stdc_first_leading_zero, (x))
#define stdc_first_leading_one(x)   __STDBIT_GENERIC(stdc_first_leading_one, (x))
#define stdc_first_trailing_zero(x) __STDBIT_GENERIC(stdc_first_trailing_zero, (x))
#define stdc_first_trailing_one(x)  __STDBIT_GENERIC(stdc_first_trailing_one, (x))
#define stdc_count_zeros(x)         __STDBIT_GENERIC(stdc_count_zeros, (x))
#define stdc_count_ones(x)          __STDBIT_GENERIC(stdc_count_ones, (x))
#define stdc_has_single_bit(x)      __STDBIT_GENERIC(stdc_has_single_bit, (x))
#define stdc_bit_width(x)           __STDBIT_GENERIC(stdc_bit_width, (x))
#define stdc_bit_floor(x)           __STDBIT_GENERIC(stdc_bit_floor, (x))
#define stdc_bit_ceil(x)            __STDBIT_GENERIC(stdc_bit_ceil, (x))

#ifdef __cplusplus
}
#endif

#endif /* _STDBIT_H */
