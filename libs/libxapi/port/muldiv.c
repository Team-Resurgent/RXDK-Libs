#include "bridge_k32.h"
/* RXDK replacement for vendor k32/i386/muldiv.asm (MulDiv). */

#include <xboxkrnl/xboxdef.h>

LONG __stdcall MulDiv(LONG nNumber, LONG nNumerator, LONG nDenominator)
{
    LONGLONG product;
    LONGLONG half;
    LONGLONG result;

    if (nDenominator == 0) {
        return -1;
    }

    product = (LONGLONG)nNumber * (LONGLONG)nNumerator;
    half = (LONGLONG)(nDenominator / 2);

    if ((product < 0) ^ (nDenominator < 0)) {
        product -= half;
    } else {
        product += half;
    }

    result = product / (LONGLONG)nDenominator;

    if (result > 0x7FFFFFFFL || result < -0x7FFFFFFFL - 1) {
        return -1;
    }

    return (LONG)result;
}
