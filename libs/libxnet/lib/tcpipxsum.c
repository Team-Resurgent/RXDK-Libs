/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * The internet checksum routine (RFC 1071): the one's-complement sum of the
 * buffer's 16-bit words, added to the initial value `xsum` and folded to 16
 * bits. Handles an unaligned start (first byte taken as the high byte of a
 * word, result byte-swapped at the end) so the result is bit-identical for any
 * buffer alignment.
 *
 * Callers complement the low 16 bits, e.g.
 *   field = (WORD) ~tcpipxsum(0, buf, len);   // COMPUTE_CHECKSUM (netutil.h)
 */

typedef unsigned long  ULONG;
typedef unsigned int   UINT;

ULONG tcpipxsum(ULONG xsum, const void *buf, UINT len)
{
    const unsigned char *p = (const unsigned char *)buf;
    ULONG sum = xsum;
    int   swapped = 0;

    if (len == 0)
        return sum;

    /* Unaligned start: fold the first byte in as the high byte of a word,
       then continue on an even boundary; swap the 16-bit result at the end. */
    if (((unsigned)(unsigned long)p & 1) != 0) {
        sum += (ULONG)p[0] << 8;
        p++;
        len--;
        swapped = 1;
    }

    while (len >= 2) {
        sum += *(const unsigned short *)p;   /* native (little-endian) 16-bit word */
        p += 2;
        len -= 2;
    }

    if (len)                                 /* trailing odd byte -> low byte */
        sum += (ULONG)p[0];

    while (sum >> 16)                        /* fold carries into 16 bits */
        sum = (sum & 0xFFFF) + (sum >> 16);

    if (swapped)
        sum = ((sum & 0x00FF) << 8) | ((sum >> 8) & 0x00FF);

    return sum;
}
