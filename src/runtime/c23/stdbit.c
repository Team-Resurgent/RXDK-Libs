#include <stdint.h>

unsigned int stdc_leading_zeros(unsigned int value)
{
    if (value == 0)
        return 32;
    unsigned int n = 0;
    while ((value & 0x80000000u) == 0) {
        value <<= 1;
        ++n;
    }
    return n;
}

unsigned int stdc_trailing_zeros(unsigned int value)
{
    if (value == 0)
        return 32;
    unsigned int n = 0;
    while ((value & 1u) == 0) {
        value >>= 1;
        ++n;
    }
    return n;
}
