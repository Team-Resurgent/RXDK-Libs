/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * MulDiv: computes (nNumber * nNumerator) / nDenominator with 64-bit
 * intermediate precision and round-to-nearest, returning -1 on a zero
 * denominator or 32-bit overflow. C reimplementation of the original i386
 * assembly.
 */

#include "bridge_k32.h"

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
