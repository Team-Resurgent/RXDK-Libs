#ifndef _STDBIT_H
#define _STDBIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int stdc_leading_zeros(unsigned int value);
unsigned int stdc_trailing_zeros(unsigned int value);

#ifdef __cplusplus
}
#endif

#endif
