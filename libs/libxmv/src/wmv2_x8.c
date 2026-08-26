/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

//------------------------------------------------------------------------------
// wmv2_x8.c -- IntraX8 (WMV2 "J-frame" / XINTRA8) keyframe decoder (see
// wmv2_x8.h). Ported from FFmpeg libavcodec/intrax8.c + intrax8dsp.c +
// wmv2dsp.c (LGPL); bit reads go through the XMV kernel's walker
// (ReadBits/ReadOneBit) and VLCs through wmv2_vlc's canonical-from-lengths
// reader (same semantics as ff_vlc_init_from_lengths). The IDCT is the WMV2
// integer IDCT ported verbatim (identity coefficient permutation), because X8
// prediction feeds decoded pixels back into the predictors -- "a +-1 idct
// error may break decoding".
//------------------------------------------------------------------------------

#include <xtl.h>
#include <stdint.h>

#include "decoder.h"        // XmvVideoCore + bit walker
#include "wmv2_vlc.h"
#include "wmv2_x8.h"
#include "wmv2_x8_tables.h" // FFmpeg intrax8huf.h tables, verbatim

#define X8MIN(a, b) ((a) < (b) ? (a) : (b))
#define X8MAX(a, b) ((a) > (b) ? (a) : (b))
#define X8ABS(a)    ((a) >= 0 ? (a) : -(a))

static uint8_t x8_clip_uint8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

// ---------------------------------------------------------------------------
// WMV2 integer IDCT (FFmpeg wmv2dsp.c, verbatim; FF_IDCT_PERM_NONE).
// ---------------------------------------------------------------------------

#define XW0 2048
#define XW1 2841
#define XW2 2676
#define XW3 2408
#define XW4 2048
#define XW5 1609
#define XW6 1108
#define XW7 565

static void x8_wmv2_idct_row(int16_t *b)
{
    int s1, s2;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    a1 = XW1 * b[1] + XW7 * b[7];
    a7 = XW7 * b[1] - XW1 * b[7];
    a5 = XW5 * b[5] + XW3 * b[3];
    a3 = XW3 * b[5] - XW5 * b[3];
    a2 = XW2 * b[2] + XW6 * b[6];
    a6 = XW6 * b[2] - XW2 * b[6];
    a0 = XW0 * b[0] + XW0 * b[4];
    a4 = XW0 * b[0] - XW0 * b[4];

    s1 = (int)(181u * (a1 - a5 + a7 - a3) + 128) >> 8;
    s2 = (int)(181u * (a1 - a5 - a7 + a3) + 128) >> 8;

    b[0] = (int16_t)((a0 + a2 + a1 + a5 + (1 << 7)) >> 8);
    b[1] = (int16_t)((a4 + a6 + s1      + (1 << 7)) >> 8);
    b[2] = (int16_t)((a4 - a6 + s2      + (1 << 7)) >> 8);
    b[3] = (int16_t)((a0 - a2 + a7 + a3 + (1 << 7)) >> 8);
    b[4] = (int16_t)((a0 - a2 - a7 - a3 + (1 << 7)) >> 8);
    b[5] = (int16_t)((a4 - a6 - s2      + (1 << 7)) >> 8);
    b[6] = (int16_t)((a4 + a6 - s1      + (1 << 7)) >> 8);
    b[7] = (int16_t)((a0 + a2 - a1 - a5 + (1 << 7)) >> 8);
}

static void x8_wmv2_idct_col(int16_t *b)
{
    int s1, s2;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    a1 = (XW1 * b[8 * 1] + XW7 * b[8 * 7] + 4) >> 3;
    a7 = (XW7 * b[8 * 1] - XW1 * b[8 * 7] + 4) >> 3;
    a5 = (XW5 * b[8 * 5] + XW3 * b[8 * 3] + 4) >> 3;
    a3 = (XW3 * b[8 * 5] - XW5 * b[8 * 3] + 4) >> 3;
    a2 = (XW2 * b[8 * 2] + XW6 * b[8 * 6] + 4) >> 3;
    a6 = (XW6 * b[8 * 2] - XW2 * b[8 * 6] + 4) >> 3;
    a0 = (XW0 * b[8 * 0] + XW0 * b[8 * 4]    ) >> 3;
    a4 = (XW0 * b[8 * 0] - XW0 * b[8 * 4]    ) >> 3;

    s1 = (int)(181u * (a1 - a5 + a7 - a3) + 128) >> 8;
    s2 = (int)(181u * (a1 - a5 - a7 + a3) + 128) >> 8;

    b[8 * 0] = (int16_t)((a0 + a2 + a1 + a5 + (1 << 13)) >> 14);
    b[8 * 1] = (int16_t)((a4 + a6 + s1      + (1 << 13)) >> 14);
    b[8 * 2] = (int16_t)((a4 - a6 + s2      + (1 << 13)) >> 14);
    b[8 * 3] = (int16_t)((a0 - a2 + a7 + a3 + (1 << 13)) >> 14);
    b[8 * 4] = (int16_t)((a0 - a2 - a7 - a3 + (1 << 13)) >> 14);
    b[8 * 5] = (int16_t)((a4 - a6 - s2      + (1 << 13)) >> 14);
    b[8 * 6] = (int16_t)((a4 + a6 - s1      + (1 << 13)) >> 14);
    b[8 * 7] = (int16_t)((a0 + a2 - a1 - a5 + (1 << 13)) >> 14);
}

static void x8_wmv2_idct_add(uint8_t *dest, int line_size, int16_t *block)
{
    int i;

    for (i = 0; i < 64; i += 8)
        x8_wmv2_idct_row(block + i);
    for (i = 0; i < 8; i++)
        x8_wmv2_idct_col(block + i);

    for (i = 0; i < 8; i++) {
        dest[0] = x8_clip_uint8(dest[0] + block[0]);
        dest[1] = x8_clip_uint8(dest[1] + block[1]);
        dest[2] = x8_clip_uint8(dest[2] + block[2]);
        dest[3] = x8_clip_uint8(dest[3] + block[3]);
        dest[4] = x8_clip_uint8(dest[4] + block[4]);
        dest[5] = x8_clip_uint8(dest[5] + block[5]);
        dest[6] = x8_clip_uint8(dest[6] + block[6]);
        dest[7] = x8_clip_uint8(dest[7] + block[7]);
        dest += line_size;
        block += 8;
    }
}

// ---------------------------------------------------------------------------
// Scantables (FFmpeg ff_wmv1_scantable[0]/[2]/[3]; identity idct permutation,
// so scan position k maps to block[scan[k]] directly).
// ---------------------------------------------------------------------------

static const uint8_t x8_scan_0[64] = { // ff_wmv1_scantable[0]
    0x00,0x08,0x01,0x02,0x09,0x10,0x18,0x11, 0x0A,0x03,0x04,0x0B,0x12,0x19,0x20,0x28,
    0x30,0x38,0x29,0x21,0x1A,0x13,0x0C,0x05, 0x06,0x0D,0x14,0x1B,0x22,0x31,0x39,0x3A,
    0x32,0x2A,0x23,0x1C,0x15,0x0E,0x07,0x0F, 0x16,0x1D,0x24,0x2B,0x33,0x3B,0x3C,0x34,
    0x2C,0x25,0x1E,0x17,0x1F,0x26,0x2D,0x35, 0x3D,0x3E,0x36,0x2E,0x27,0x2F,0x37,0x3F,
};
static const uint8_t x8_scan_2[64] = { // ff_wmv1_scantable[2] (horizontal)
    0x00,0x01,0x08,0x02,0x03,0x09,0x10,0x18, 0x11,0x0A,0x04,0x05,0x0B,0x12,0x19,0x20,
    0x28,0x30,0x21,0x1A,0x13,0x0C,0x06,0x07, 0x0D,0x14,0x1B,0x22,0x29,0x38,0x31,0x39,
    0x2A,0x23,0x1C,0x15,0x0E,0x0F,0x16,0x1D, 0x24,0x2B,0x32,0x3A,0x33,0x2C,0x25,0x1E,
    0x17,0x1F,0x26,0x2D,0x34,0x3B,0x3C,0x35, 0x2E,0x27,0x2F,0x36,0x3D,0x3E,0x37,0x3F,
};
static const uint8_t x8_scan_3[64] = { // ff_wmv1_scantable[3] (vertical)
    0x00,0x08,0x10,0x01,0x18,0x20,0x28,0x09, 0x02,0x03,0x0A,0x11,0x19,0x30,0x38,0x29,
    0x21,0x1A,0x12,0x0B,0x04,0x05,0x0C,0x13, 0x1B,0x22,0x31,0x39,0x32,0x2A,0x23,0x1C,
    0x14,0x0D,0x06,0x07,0x0E,0x15,0x1D,0x24, 0x2B,0x33,0x3A,0x3B,0x34,0x2C,0x25,0x1E,
    0x16,0x0F,0x17,0x1F,0x26,0x2D,0x3C,0x35, 0x2E,0x27,0x2F,0x36,0x3D,0x3E,0x37,0x3F,
};
static const uint8_t *const x8_scantables[3] = { x8_scan_0, x8_scan_2, x8_scan_3 };

// ---------------------------------------------------------------------------
// Static VLC sets, built once (mirrors intrax8.c's x8_vlc_init).
// ---------------------------------------------------------------------------

static Wmv2Vlc g_x8_ac_vlc[2][2][8]; // [quant < 13][intra/inter][select]
static Wmv2Vlc g_x8_dc_vlc[2][8];    // [quant < 13][select]
static Wmv2Vlc g_x8_or_vlc[2][4];    // [quant < 13][select] ([0] has 2 tables)
static int g_x8_vlc_built = 0;

static int x8_build_vlc(Wmv2Vlc *v, const uint8_t (*tab)[2], int n)
{
    uint8_t  lens[128];
    uint16_t syms[128];
    int i;
    for (i = 0; i < n; i++) {
        syms[i] = tab[i][0];
        lens[i] = tab[i][1];
    }
    return Wmv2VlcBuildFromLengths(v, lens, syms, n);
}

static int x8_vlc_init_once(void)
{
    int i, j, k;

    if (g_x8_vlc_built)
        return 0;

    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 8; k++)
                if (x8_build_vlc(&g_x8_ac_vlc[i][j][k], x8_ac_quant_table[i][j][k], 77) != 0)
                    return -1;

    for (i = 0; i < 2; i++)
        for (j = 0; j < 8; j++)
            if (x8_build_vlc(&g_x8_dc_vlc[i][j], x8_dc_quant_table[i][j], 34) != 0)
                return -1;

    for (i = 0; i < 2; i++)
        if (x8_build_vlc(&g_x8_or_vlc[0][i], x8_orient_highquant_table[i], 12) != 0)
            return -1;
    for (i = 0; i < 4; i++)
        if (x8_build_vlc(&g_x8_or_vlc[1][i], x8_orient_lowquant_table[i], 12) != 0)
            return -1;

    g_x8_vlc_built = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Bits-left tracking over the XMV walker.
// ---------------------------------------------------------------------------

static int x8_bits_left(const Wmv2X8 *w)
{
    const XmvVideoCore *c = w->core;
    return (int)((w->bit_end - c->pDecodingPosition) * 8) + (int)c->BitsRemaining;
}

// ---------------------------------------------------------------------------
// Edge setup + 12 spatial predictors + loop filter (intrax8dsp.c, ported).
// The scratchpad layout matches FFmpeg's emu-edge convention:
//   area1[0..7]  left column of the block left of the left neighbour
//   area2[8..15] left neighbour's right column (bottom-up)
//   area3[16]    top-left corner pixel
//   area4[17..]  top row (16 pixels: above + above-right)
//   area6[33..]  row above the top row (8 pixels)
// ---------------------------------------------------------------------------

#define area1 (0)
#define area2 (8)
#define area3 (8 + 8)
#define area4 (8 + 8 + 1)
#define area5 (8 + 8 + 1 + 8)
#define area6 (8 + 8 + 1 + 16)

static void x8_setup_spatial_compensation(const uint8_t *src, uint8_t *dst,
                                          int stride, int *range,
                                          int *psum, int edges)
{
    const uint8_t *ptr;
    int sum;
    int i;
    int min_pix, max_pix;
    uint8_t c = 0;

    if ((edges & 3) == 3) {
        *psum  = 0x80 * (8 + 1 + 8 + 2);
        *range = 0;
        memset(dst, 0x80, 16 + 1 + 16 + 8);
        return;
    }

    min_pix = 256;
    max_pix = -1;

    sum = 0;

    if (!(edges & 1)) { // (mb_x != 0)
        ptr = src - 1;
        for (i = 7; i >= 0; i--) {
            c              = *(ptr - 1);
            dst[area1 + i] = c;
            c              = *ptr;

            sum           += c;
            min_pix        = X8MIN(min_pix, c);
            max_pix        = X8MAX(max_pix, c);
            dst[area2 + i] = c;

            ptr += stride;
        }
    }

    if (!(edges & 2)) { // (mb_y != 0)
        ptr = src - stride;
        for (i = 0; i < 8; i++) {
            c       = *(ptr + i);
            sum    += c;
            min_pix = X8MIN(min_pix, (int)c);
            max_pix = X8MAX(max_pix, (int)c);
        }
        if (edges & 4) { // last block on the row?
            memset(dst + area5, c, 8);
            memcpy(dst + area4, ptr, 8);
        } else {
            memcpy(dst + area4, ptr, 16);
        }
        memcpy(dst + area6, ptr - stride, 8);
    }

    if (edges & 3) {
        int avg = (sum + 4) >> 3;

        if (edges & 1)
            memset(dst + area1, avg, 8 + 8 + 1);
        else
            memset(dst + area3, avg, 1 + 16 + 8);

        sum += avg * 9;
    } else {
        uint8_t cc = *(src - 1 - stride);
        dst[area3] = cc;
        sum       += cc;
    }
    *range = max_pix - min_pix;
    sum   += *(dst + area5) + *(dst + area5 + 1);
    *psum  = sum;
}

static const uint16_t x8_zero_prediction_weights[64 * 2] = {
    640,  640, 669,  480, 708,  354, 748, 257,
    792,  198, 760,  143, 808,  101, 772,  72,
    480,  669, 537,  537, 598,  416, 661, 316,
    719,  250, 707,  185, 768,  134, 745,  97,
    354,  708, 416,  598, 488,  488, 564, 388,
    634,  317, 642,  241, 716,  179, 706, 132,
    257,  748, 316,  661, 388,  564, 469, 469,
    543,  395, 571,  311, 655,  238, 660, 180,
    198,  792, 250,  719, 317,  634, 395, 543,
    469,  469, 507,  380, 597,  299, 616, 231,
    161,  855, 206,  788, 266,  710, 340, 623,
    411,  548, 455,  455, 548,  366, 576, 288,
    122,  972, 159,  914, 211,  842, 276, 758,
    341,  682, 389,  584, 483,  483, 520, 390,
    110, 1172, 144, 1107, 193, 1028, 254, 932,
    317,  846, 366,  731, 458,  611, 499, 499,
};

static void spatial_compensation_0(const uint8_t *src, uint8_t *dst, int stride)
{
    int i, j;
    int x, y;
    unsigned int p;
    int a;
    uint16_t left_sum[2][8];
    uint16_t top_sum[2][8];

    memset(left_sum, 0, sizeof(left_sum));
    memset(top_sum, 0, sizeof(top_sum));

    for (i = 0; i < 8; i++) {
        a = src[area2 + 7 - i] << 4;
        for (j = 0; j < 8; j++) {
            p                   = (unsigned)X8ABS(i - j);
            left_sum[p & 1][j] = (uint16_t)(left_sum[p & 1][j] + (a >> (p >> 1)));
        }
    }

    for (i = 0; i < 8; i++) {
        a = src[area4 + i] << 4;
        for (j = 0; j < 8; j++) {
            p                  = (unsigned)X8ABS(i - j);
            top_sum[p & 1][j] = (uint16_t)(top_sum[p & 1][j] + (a >> (p >> 1)));
        }
    }
    for (; i < 10; i++) {
        a = src[area4 + i] << 4;
        for (j = 5; j < 8; j++) {
            p                  = (unsigned)X8ABS(i - j);
            top_sum[p & 1][j] = (uint16_t)(top_sum[p & 1][j] + (a >> (p >> 1)));
        }
    }
    for (; i < 12; i++) {
        a = src[area4 + i] << 4;
        for (j = 7; j < 8; j++) {
            p                  = (unsigned)X8ABS(i - j);
            top_sum[p & 1][j] = (uint16_t)(top_sum[p & 1][j] + (a >> (p >> 1)));
        }
    }

    for (i = 0; i < 8; i++) {
        top_sum[0][i]  = (uint16_t)(top_sum[0][i]  + ((top_sum[1][i]  * 181 + 128) >> 8));
        left_sum[0][i] = (uint16_t)(left_sum[0][i] + ((left_sum[1][i] * 181 + 128) >> 8));
    }
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = (uint8_t)(((uint32_t) top_sum[0][x]  * x8_zero_prediction_weights[y * 16 + x * 2 + 0] +
                                (uint32_t) left_sum[0][y] * x8_zero_prediction_weights[y * 16 + x * 2 + 1] +
                                0x8000) >> 16);
        dst += stride;
    }
}

static void spatial_compensation_1(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = src[area4 + X8MIN(2 * y + x + 2, 15)];
        dst += stride;
    }
}

static void spatial_compensation_2(const uint8_t *src, uint8_t *dst, int stride)
{
    int y;

    for (y = 0; y < 8; y++) {
        memcpy(dst, src + area4 + 1 + y, 8);
        dst += stride;
    }
}

static void spatial_compensation_3(const uint8_t *src, uint8_t *dst, int stride)
{
    int y;

    for (y = 0; y < 8; y++) {
        memcpy(dst, src + area4 + ((y + 1) >> 1), 8);
        dst += stride;
    }
}

static void spatial_compensation_4(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = (uint8_t)((src[area4 + x] + src[area6 + x] + 1) >> 1);
        dst += stride;
    }
}

static void spatial_compensation_5(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            if (2 * x - y < 0)
                dst[x] = src[area2 + 9 + 2 * x - y];
            else
                dst[x] = src[area4 + x - ((y + 1) >> 1)];
        }
        dst += stride;
    }
}

static void spatial_compensation_6(const uint8_t *src, uint8_t *dst, int stride)
{
    int y;

    for (y = 0; y < 8; y++) {
        memcpy(dst, src + area3 - y, 8);
        dst += stride;
    }
}

static void spatial_compensation_7(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            if (x - 2 * y > 0)
                dst[x] = (uint8_t)((src[area3 - 1 + x - 2 * y] + src[area3 + x - 2 * y] + 1) >> 1);
            else
                dst[x] = src[area2 + 8 - y + (x >> 1)];
        }
        dst += stride;
    }
}

static void spatial_compensation_8(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = (uint8_t)((src[area1 + 7 - y] + src[area2 + 7 - y] + 1) >> 1);
        dst += stride;
    }
}

static void spatial_compensation_9(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = src[area2 + 6 - X8MIN(x + y, 6)];
        dst += stride;
    }
}

static void spatial_compensation_10(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = (uint8_t)((src[area2 + 7 - y] * (8 - x) + src[area4 + x] * x + 4) >> 3);
        dst += stride;
    }
}

static void spatial_compensation_11(const uint8_t *src, uint8_t *dst, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            dst[x] = (uint8_t)((src[area2 + 7 - y] * y + src[area4 + x] * (8 - y) + 4) >> 3);
        dst += stride;
    }
}

typedef void (*X8SpatialFn)(const uint8_t *src, uint8_t *dst, int stride);

static const X8SpatialFn x8_spatial_compensation[12] = {
    spatial_compensation_0,  spatial_compensation_1,  spatial_compensation_2,
    spatial_compensation_3,  spatial_compensation_4,  spatial_compensation_5,
    spatial_compensation_6,  spatial_compensation_7,  spatial_compensation_8,
    spatial_compensation_9,  spatial_compensation_10, spatial_compensation_11,
};

static void x8_loop_filter(uint8_t *ptr, const int a_stride,
                           const int b_stride, int quant)
{
    int i, t;
    int p0, p1, p2, p3, p4, p5, p6, p7, p8, p9;
    int ql = (quant + 10) >> 3;

    for (i = 0; i < 8; i++, ptr += b_stride) {
        p0 = ptr[-5 * a_stride];
        p1 = ptr[-4 * a_stride];
        p2 = ptr[-3 * a_stride];
        p3 = ptr[-2 * a_stride];
        p4 = ptr[-1 * a_stride];
        p5 = ptr[0];
        p6 = ptr[1 * a_stride];
        p7 = ptr[2 * a_stride];
        p8 = ptr[3 * a_stride];
        p9 = ptr[4 * a_stride];

        t = (X8ABS(p1 - p2) <= ql) +
            (X8ABS(p2 - p3) <= ql) +
            (X8ABS(p3 - p4) <= ql) +
            (X8ABS(p4 - p5) <= ql);

        if (t > 0) {
            t += (X8ABS(p5 - p6) <= ql) +
                 (X8ABS(p6 - p7) <= ql) +
                 (X8ABS(p7 - p8) <= ql) +
                 (X8ABS(p8 - p9) <= ql) +
                 (X8ABS(p0 - p1) <= ql);
            if (t >= 6) {
                int min, max;

                min = max = p1;
                min = X8MIN(min, p3);
                max = X8MAX(max, p3);
                min = X8MIN(min, p5);
                max = X8MAX(max, p5);
                min = X8MIN(min, p8);
                max = X8MAX(max, p8);
                if (max - min < 2 * quant) {
                    min = X8MIN(min, p2);
                    max = X8MAX(max, p2);
                    min = X8MIN(min, p4);
                    max = X8MAX(max, p4);
                    min = X8MIN(min, p6);
                    max = X8MAX(max, p6);
                    min = X8MIN(min, p7);
                    max = X8MAX(max, p7);
                    if (max - min < 2 * quant) {
                        ptr[-2 * a_stride] = (uint8_t)((4 * p2 + 3 * p3 + 1 * p7 + 4) >> 3);
                        ptr[-1 * a_stride] = (uint8_t)((3 * p2 + 3 * p4 + 2 * p7 + 4) >> 3);
                        ptr[0]             = (uint8_t)((2 * p2 + 3 * p5 + 3 * p7 + 4) >> 3);
                        ptr[1 * a_stride]  = (uint8_t)((1 * p2 + 3 * p6 + 4 * p7 + 4) >> 3);
                        continue;
                    }
                }
            }
        }
        {
            int x, x0, x1, x2;
            int m;

            x0 = (2 * p3 - 5 * p4 + 5 * p5 - 2 * p6 + 4) >> 3;
            if (X8ABS(x0) < quant) {
                x1 = (2 * p1 - 5 * p2 + 5 * p3 - 2 * p4 + 4) >> 3;
                x2 = (2 * p5 - 5 * p6 + 5 * p7 - 2 * p8 + 4) >> 3;

                x = X8ABS(x0) - X8MIN(X8ABS(x1), X8ABS(x2));
                m = p4 - p5;

                if (x > 0 && (m ^ x0) < 0) {
                    int32_t sign;

                    sign = m >> 31;
                    m    = (m ^ sign) - sign;
                    m  >>= 1;

                    x = 5 * x >> 3;

                    if (x > m)
                        x = m;

                    x = (x ^ sign) - sign;

                    ptr[-1 * a_stride] = (uint8_t)(ptr[-1 * a_stride] - x);
                    ptr[0]             = (uint8_t)(ptr[0] + x);
                }
            }
        }
    }
}

static void x8_h_loop_filter(uint8_t *src, int stride, int qscale)
{
    x8_loop_filter(src, stride, 1, qscale);
}

static void x8_v_loop_filter(uint8_t *src, int stride, int qscale)
{
    x8_loop_filter(src, 1, stride, qscale);
}

// ---------------------------------------------------------------------------
// VLC-driven symbol decode (intrax8.c, ported).
// ---------------------------------------------------------------------------

static void x8_select_ac_table(Wmv2X8 *w, int mode)
{
    int table_index;

    if (w->ac_vlc[mode])
        return;

    table_index     = (int)ReadBits(w->core, 3);
    // 2 modes use same tables
    w->ac_vlc[mode] = &g_x8_ac_vlc[w->quant < 13][mode >> 1][table_index];
}

static int x8_get_orient_vlc(Wmv2X8 *w)
{
    if (!w->orient_vlc) {
        int table_index = (int)ReadBits(w->core, 1 + (w->quant < 13));
        w->orient_vlc = &g_x8_or_vlc[w->quant < 13][table_index];
    }

    return Wmv2VlcDecode(w->core, (const Wmv2Vlc *)w->orient_vlc);
}

#define x8_extra_bits(eb)  (eb)        // 3 bits
#define x8_extra_run       (0xFF << 8) // 1 bit
#define x8_extra_level     (0x00 << 8) // 1 bit
#define x8_run_offset(r)   ((r) << 16) // 6 bits
#define x8_level_offset(l) ((l) << 24) // 5 bits
static const uint32_t x8_ac_decode_table[] = {
    /* 46 */ x8_extra_bits(3) | x8_extra_run   | x8_run_offset(16) | x8_level_offset(0),
    /* 47 */ x8_extra_bits(3) | x8_extra_run   | x8_run_offset(24) | x8_level_offset(0),
    /* 48 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(4)  | x8_level_offset(1),
    /* 49 */ x8_extra_bits(3) | x8_extra_run   | x8_run_offset(8)  | x8_level_offset(1),

    /* 50 */ x8_extra_bits(5) | x8_extra_run   | x8_run_offset(32) | x8_level_offset(0),
    /* 51 */ x8_extra_bits(4) | x8_extra_run   | x8_run_offset(16) | x8_level_offset(1),

    /* 52 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(4),
    /* 53 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(8),
    /* 54 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(12),
    /* 55 */ x8_extra_bits(3) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(16),
    /* 56 */ x8_extra_bits(3) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(24),

    /* 57 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(1)  | x8_level_offset(3),
    /* 58 */ x8_extra_bits(3) | x8_extra_level | x8_run_offset(1)  | x8_level_offset(7),

    /* 59 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(16) | x8_level_offset(0),
    /* 60 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(20) | x8_level_offset(0),
    /* 61 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(24) | x8_level_offset(0),
    /* 62 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(28) | x8_level_offset(0),
    /* 63 */ x8_extra_bits(4) | x8_extra_run   | x8_run_offset(32) | x8_level_offset(0),
    /* 64 */ x8_extra_bits(4) | x8_extra_run   | x8_run_offset(48) | x8_level_offset(0),

    /* 65 */ x8_extra_bits(2) | x8_extra_run   | x8_run_offset(4)  | x8_level_offset(1),
    /* 66 */ x8_extra_bits(3) | x8_extra_run   | x8_run_offset(8)  | x8_level_offset(1),
    /* 67 */ x8_extra_bits(4) | x8_extra_run   | x8_run_offset(16) | x8_level_offset(1),

    /* 68 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(4),
    /* 69 */ x8_extra_bits(3) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(8),
    /* 70 */ x8_extra_bits(4) | x8_extra_level | x8_run_offset(0)  | x8_level_offset(16),

    /* 71 */ x8_extra_bits(2) | x8_extra_level | x8_run_offset(1)  | x8_level_offset(3),
    /* 72 */ x8_extra_bits(3) | x8_extra_level | x8_run_offset(1)  | x8_level_offset(7),
};

static void x8_get_ac_rlf(Wmv2X8 *w, const int mode,
                          int *const run, int *const level, int *const final)
{
    int i, e;

    i = Wmv2VlcDecode(w->core, (const Wmv2Vlc *)w->ac_vlc[mode]);

    if (i < 46) { // [0-45]
        int t, l;
        if (i < 0) {
            *level =
            *final =
            *run   = 64;
            return;
        }

        *final =
        t      = i > 22;
        i     -= 23 * t;

        l = (0xE50000 >> (i & 0x1E)) & 3;

        t = 0x01030F >> (l << 3);

        *run   = i & t;
        *level = l;
    } else if (i < 73) { // [46-72]
        uint32_t sm;
        uint32_t mask;

        i -= 46;
        sm = x8_ac_decode_table[i];

        e    = (int)ReadBits(w->core, sm & 0xF);
        sm >>= 8;
        mask = sm & 0xff;
        sm >>= 8;

        *run   = (sm &  0xff) + (e &  (int)mask);
        *level = (sm >>    8) + (e & ~(int)mask);
        *final = i > (58 - 46);
    } else if (i < 75) { // [73-74]
        static const uint8_t crazy_mix_runlevel[32] = {
            0x22, 0x32, 0x33, 0x53, 0x23, 0x42, 0x43, 0x63,
            0x24, 0x52, 0x34, 0x73, 0x25, 0x62, 0x44, 0x83,
            0x26, 0x72, 0x35, 0x54, 0x27, 0x82, 0x45, 0x64,
            0x28, 0x92, 0x36, 0x74, 0x29, 0xa2, 0x46, 0x84,
        };

        *final = !(i & 1);
        e      = (int)ReadBits(w->core, 5);
        *run   = crazy_mix_runlevel[e] >> 4;
        *level = crazy_mix_runlevel[e] & 0x0F;
    } else {
        *level = (int)ReadBits(w->core, 7 - 3 * (i & 1));
        *run   = (int)ReadBits(w->core, 6);
        *final = (int)ReadOneBit(w->core);
    }
}

static const uint8_t x8_dc_index_offset[] = {
    0, 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
};

static int x8_get_dc_rlf(Wmv2X8 *w, const int mode,
                         int *const level, int *const final)
{
    int i, e, c;

    if (!w->dc_vlc[mode]) {
        int table_index = (int)ReadBits(w->core, 3);
        // 4 modes, same table
        w->dc_vlc[mode] = &g_x8_dc_vlc[w->quant < 13][table_index];
    }

    i = Wmv2VlcDecode(w->core, (const Wmv2Vlc *)w->dc_vlc[mode]);

    c      = i > 16;
    *final = c;
    i     -= 17 * c;

    if (i <= 0) {
        *level = 0;
        return -i;
    }
    c  = (i + 1) >> 1;
    c -= c > 1;

    e = (int)ReadBits(w->core, c);
    i = x8_dc_index_offset[i] + (e >> 1);

    e      = -(e & 1);
    *level =  (i ^ e) - e;
    return 0;
}

// ---------------------------------------------------------------------------
// Prediction bookkeeping (intrax8.c, ported).
// ---------------------------------------------------------------------------

static int x8_setup_spatial_predictor(Wmv2X8 *w, const int chroma)
{
    int range;
    int sum;
    int quant;

    x8_setup_spatial_compensation(w->dest[chroma], w->scratchpad,
                                  w->linesize[chroma > 0],
                                  &range, &sum, w->edges);
    if (chroma) {
        w->orient = w->chroma_orient;
        quant     = w->quant_dc_chroma;
    } else {
        quant = w->quant;
    }

    w->flat_dc = 0;
    if (range < quant || range < 3) {
        w->orient = 0;

        // yep you read right, a +-1 idct error may break decoding!
        if (range < 3) {
            w->flat_dc      = 1;
            sum            += 9;
            // ((1 << 17) + 9) / (8 + 8 + 1 + 2) = 6899
            w->predicted_dc = sum * 6899 >> 17;
        }
    }
    if (chroma)
        return 0;

    if (range < 2 * w->quant) {
        if ((w->edges & 3) == 0) {
            if (w->orient == 1)
                w->orient = 11;
            if (w->orient == 2)
                w->orient = 10;
        } else {
            w->orient = 0;
        }
        w->raw_orient = 0;
    } else {
        static const uint8_t prediction_table[3][12] = {
            { 0, 8, 4, 10, 11, 2, 6, 9, 1, 3, 5, 7 },
            { 4, 0, 8, 11, 10, 3, 5, 2, 6, 9, 1, 7 },
            { 8, 0, 4, 10, 11, 1, 7, 2, 6, 9, 3, 5 },
        };
        w->raw_orient = x8_get_orient_vlc(w);
        if (w->raw_orient < 0 || w->raw_orient >= 12)
            return -1;
        w->orient = prediction_table[w->orient][w->raw_orient];
    }
    return 0;
}

static void x8_update_predictions(Wmv2X8 *w, const int orient, const int est_run)
{
    w->prediction_table[w->mb_x * 2 + (w->mb_y & 1)] =
        (uint8_t)((est_run << 2) + 1 * (orient == 4) + 2 * (orient == 8));
}

static void x8_get_prediction_chroma(Wmv2X8 *w)
{
    w->edges  = 1 * !(w->mb_x >> 1);
    w->edges |= 2 * !(w->mb_y >> 1);
    w->edges |= 4 * (w->mb_x >= (2 * w->mb_width - 1)); // mb_x for chroma would always be odd

    w->raw_orient = 0;
    if (w->edges & 3) {
        w->chroma_orient = 4 << ((0xCC >> w->edges) & 1);
        return;
    }
    w->chroma_orient = (w->prediction_table[2 * w->mb_x - 2] & 0x03) << 2;
}

static void x8_get_prediction(Wmv2X8 *w)
{
    int a, b, c, i;

    w->edges  = 1 * !w->mb_x;
    w->edges |= 2 * !w->mb_y;
    w->edges |= 4 * (w->mb_x >= (2 * w->mb_width - 1));

    switch (w->edges & 3) {
    case 0:
        break;
    case 1:
        // take the one from the above block[0][y - 1]
        w->est_run = w->prediction_table[!(w->mb_y & 1)] >> 2;
        w->orient  = 1;
        return;
    case 2:
        // take the one from the previous block[x - 1][0]
        w->est_run = w->prediction_table[2 * w->mb_x - 2] >> 2;
        w->orient  = 2;
        return;
    case 3:
        w->est_run = 16;
        w->orient  = 0;
        return;
    }
    // no edge cases
    b = w->prediction_table[2 * w->mb_x     + !(w->mb_y & 1)]; // block[x    ][y - 1]
    a = w->prediction_table[2 * w->mb_x - 2 +  (w->mb_y & 1)]; // block[x - 1][y    ]
    c = w->prediction_table[2 * w->mb_x - 2 + !(w->mb_y & 1)]; // block[x - 1][y - 1]

    w->est_run = X8MIN(b, a);
    if ((w->mb_x & w->mb_y) != 0)
        w->est_run = X8MIN(c, w->est_run);
    w->est_run >>= 2;

    a &= 3;
    b &= 3;
    c &= 3;

    i = (0xFFEAF4C4u >> (2 * b + 8 * a)) & 3;
    if (i != 3)
        w->orient = i;
    else
        w->orient = (0xFFEAD8u >> (2 * c + 8 * (w->quant > 12))) & 3;
}

// ---------------------------------------------------------------------------
// Block reconstruction (intrax8.c, ported; identity idct permutation).
// ---------------------------------------------------------------------------

static void x8_ac_compensation(Wmv2X8 *w, const int direction, const int dc_level)
{
    int t;
#define B(x, y) w->block[(x) + (y) * 8]
#define T(x)  ((x) * dc_level + 0x8000) >> 16;
    switch (direction) {
    case 0:
        t        = T(3811); // h
        B(1, 0) -= t;
        B(0, 1) -= t;

        t        = T(487); // e
        B(2, 0) -= t;
        B(0, 2) -= t;

        t        = T(506); // f
        B(3, 0) -= t;
        B(0, 3) -= t;

        t        = T(135); // c
        B(4, 0) -= t;
        B(0, 4) -= t;
        B(2, 1) += t;
        B(1, 2) += t;
        B(3, 1) += t;
        B(1, 3) += t;

        t        = T(173); // d
        B(5, 0) -= t;
        B(0, 5) -= t;

        t        = T(61); // b
        B(6, 0) -= t;
        B(0, 6) -= t;
        B(5, 1) += t;
        B(1, 5) += t;

        t        = T(42); // a
        B(7, 0) -= t;
        B(0, 7) -= t;
        B(4, 1) += t;
        B(1, 4) += t;
        B(4, 4) += t;

        t        = T(1084); // g
        B(1, 1) += t;
        break;
    case 1:
        B(0, 1) -= T(6269);
        B(0, 3) -= T(708);
        B(0, 5) -= T(172);
        B(0, 7) -= T(73);
        break;
    case 2:
        B(1, 0) -= T(6269);
        B(3, 0) -= T(708);
        B(5, 0) -= T(172);
        B(7, 0) -= T(73);
        break;
    }
#undef B
#undef T
}

static void x8_put_solidcolor(const uint8_t pix, uint8_t *dst, const int linesize)
{
    int k;
    for (k = 0; k < 8; k++) {
        memset(dst, pix, 8);
        dst += linesize;
    }
}

static const int16_t x8_quant_table[64] = {
    256, 256, 256, 256, 256, 256, 259, 262,
    265, 269, 272, 275, 278, 282, 285, 288,
    292, 295, 299, 303, 306, 310, 314, 317,
    321, 325, 329, 333, 337, 341, 345, 349,
    353, 358, 362, 366, 371, 375, 379, 384,
    389, 393, 398, 403, 408, 413, 417, 422,
    428, 433, 438, 443, 448, 454, 459, 465,
    470, 476, 482, 488, 493, 499, 505, 511,
};

static int x8_decode_intra_mb(Wmv2X8 *w, const int chroma)
{
    const uint8_t *scantable;
    int final, run, level;
    int ac_mode, dc_mode, est_run, dc_level;
    int pos, n;
    int zeros_only;
    int use_quant_matrix;
    int sign;

    memset(w->block, 0, sizeof(w->block));

    if (chroma)
        dc_mode = 2;
    else
        dc_mode = !!w->est_run; // 0, 1

    if (x8_get_dc_rlf(w, dc_mode, &dc_level, &final))
        return -1;
    n          = 0;
    zeros_only = 0;
    if (!final) { // decode ac
        use_quant_matrix = w->use_quant_matrix;
        if (chroma) {
            ac_mode = 1;
            est_run = 64; // not used
        } else {
            if (w->raw_orient < 3)
                use_quant_matrix = 0;

            if (w->raw_orient > 4) {
                ac_mode = 0;
                est_run = 64;
            } else {
                if (w->est_run > 1) {
                    ac_mode = 2;
                    est_run = w->est_run;
                } else {
                    ac_mode = 3;
                    est_run = 64;
                }
            }
        }
        x8_select_ac_table(w, ac_mode);
        /* scantable_selector[12] = { 0, 2, 0, 1, 1, 1, 0, 2, 2, 0, 1, 2 }; */
        scantable = x8_scantables[(0x928548 >> (2 * w->orient)) & 3];
        pos       = 0;
        do {
            n++;
            if (n >= est_run) {
                ac_mode = 3;
                x8_select_ac_table(w, 3);
            }

            x8_get_ac_rlf(w, ac_mode, &run, &level, &final);

            pos += run + 1;
            if (pos > 63) {
                // this also handles vlc error in x8_get_ac_rlf
                return -1;
            }
            level  = (level + 1) * w->dquant;
            level += w->qsum;

            sign  = -(int)ReadOneBit(w->core);
            level = (level ^ sign) - sign;

            if (use_quant_matrix)
                level = (level * x8_quant_table[pos]) >> 8;

            w->block[scantable[pos]] = (int16_t)level;
        } while (!final);
    } else { // DC only
        if (w->flat_dc && ((unsigned) (dc_level + 1)) < 3) { // [-1; 1]
            int32_t divide_quant = !chroma ? w->divide_quant_dc_luma
                                           : w->divide_quant_dc_chroma;
            int32_t dc_quant     = !chroma ? w->quant
                                           : w->quant_dc_chroma;

            dc_level += (w->predicted_dc * divide_quant + (1 << 12)) >> 13;

            x8_put_solidcolor(x8_clip_uint8((dc_level * dc_quant + 4) >> 3),
                              w->dest[chroma],
                              w->linesize[!!chroma]);

            goto block_placed;
        }
        zeros_only = dc_level == 0;
    }
    if (!chroma)
        w->block[0] = (int16_t)(dc_level * w->quant);
    else
        w->block[0] = (int16_t)(dc_level * w->quant_dc_chroma);

    // there is !zero_only check in the original, but dc_level check is enough
    if ((unsigned int) (dc_level + 1) >= 3 && (w->edges & 3) != 3) {
        int direction;
        /* ac_comp_direction[orient] = { 0, 3, 3, 1, 1, 0, 0, 0, 2, 2, 2, 1 }; */
        direction = (0x6A017C >> (w->orient * 2)) & 3;
        if (direction != 3) {
            x8_ac_compensation(w, direction, w->block[0]);
        }
    }

    if (w->flat_dc) {
        x8_put_solidcolor((uint8_t)w->predicted_dc, w->dest[chroma],
                          w->linesize[!!chroma]);
    } else {
        x8_spatial_compensation[w->orient](w->scratchpad,
                                           w->dest[chroma],
                                           w->linesize[!!chroma]);
    }
    if (!zeros_only)
        x8_wmv2_idct_add(w->dest[chroma],
                         w->linesize[!!chroma],
                         w->block);

block_placed:
    if (!chroma)
        x8_update_predictions(w, w->orient, n);

    if (w->loopfilter) {
        uint8_t *ptr = w->dest[chroma];
        int linesize = w->linesize[!!chroma];

        if (!((w->edges & 2) || (zeros_only && (w->orient | 4) == 4)))
            x8_h_loop_filter(ptr, linesize, w->quant);

        if (!((w->edges & 1) || (zeros_only && (w->orient | 8) == 8)))
            x8_v_loop_filter(ptr, linesize, w->quant);
    }
    return 0;
}

static void x8_init_block_index(Wmv2X8 *w)
{
    XmvVideoCore *c = w->core;
    const int linesize   = w->linesize[0];
    const int uvlinesize = w->linesize[1];

    // Decode into the BUILDING planes; the caller promotes with a swap.
    w->dest[0] = c->pYBuilding;
    w->dest[1] = c->pUBuilding;
    w->dest[2] = c->pVBuilding;

    w->dest[0] +=  w->mb_y       * linesize   << 3;
    // chroma blocks are on add rows
    w->dest[1] += (w->mb_y & ~1) * uvlinesize << 2;
    w->dest[2] += (w->mb_y & ~1) * uvlinesize << 2;
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

int Wmv2X8Init(Wmv2X8 *w, struct XmvVideoCore *core)
{
    XmvVideoCore *c = (XmvVideoCore *)core;

    memset(w, 0, sizeof(*w));
    w->core      = c;
    w->mb_width  = (int)c->MBWidth;
    w->mb_height = (int)c->MBHeight;
    w->linesize[0] = (int)(c->MBWidth * MACROBLOCK_SIZE);
    w->linesize[1] = (int)(c->MBWidth * BLOCK_SIZE);

    // two rows, 2 blocks per cannon mb
    w->prediction_table = (uint8_t *)malloc(w->mb_width * 2 * 2);
    if (!w->prediction_table)
        return -1;
    memset(w->prediction_table, 0, w->mb_width * 2 * 2);

    if (x8_vlc_init_once() != 0) {
        free(w->prediction_table);
        w->prediction_table = NULL;
        return -1;
    }

    w->initialized = 1;
    return 0;
}

void Wmv2X8Free(Wmv2X8 *w)
{
    if (w->prediction_table) {
        free(w->prediction_table);
        w->prediction_table = NULL;
    }
    w->initialized = 0;
}

int Wmv2X8DecodeFrame(Wmv2X8 *w, int qscale, int loopfilter,
                      const uint8_t *bit_end)
{
    int i;

    if (!w->initialized || qscale <= 0)
        return -1;

    // Mirrors ff_intrax8_decode_picture(w, ..., 2 * qscale, (qscale - 1) | 1,
    // loop_filter, low_delay).
    w->dquant     = 2 * qscale;
    w->quant      = qscale;
    w->qsum       = (qscale - 1) | 1;
    w->loopfilter = loopfilter;
    w->bit_end    = bit_end;
    w->use_quant_matrix = (int)ReadOneBit(w->core);

    w->divide_quant_dc_luma = ((1 << 16) + (w->quant >> 1)) / w->quant;
    if (w->quant < 5) {
        w->quant_dc_chroma        = w->quant;
        w->divide_quant_dc_chroma = w->divide_quant_dc_luma;
    } else {
        w->quant_dc_chroma        = w->quant + ((w->quant + 3) >> 3);
        w->divide_quant_dc_chroma = ((1 << 16) + (w->quant_dc_chroma >> 1)) / w->quant_dc_chroma;
    }

    // Reset the lazily-selected VLC tables.
    for (i = 0; i < 4; i++)
        w->ac_vlc[i] = NULL;
    for (i = 0; i < 3; i++)
        w->dc_vlc[i] = NULL;
    w->orient_vlc = NULL;

    for (w->mb_y = 0; w->mb_y < w->mb_height * 2; w->mb_y++) {
        x8_init_block_index(w);
        if (x8_bits_left(w) < 1)
            goto error;
        for (w->mb_x = 0; w->mb_x < w->mb_width * 2; w->mb_x++) {
            x8_get_prediction(w);
            if (x8_setup_spatial_predictor(w, 0))
                goto error;
            if (x8_decode_intra_mb(w, 0))
                goto error;

            if (w->mb_x & w->mb_y & 1) {
                x8_get_prediction_chroma(w);

                /* when setting up chroma, no vlc is read,
                 * so no error condition can be reached */
                x8_setup_spatial_predictor(w, 1);
                if (x8_decode_intra_mb(w, 1))
                    goto error;

                x8_setup_spatial_predictor(w, 2);
                if (x8_decode_intra_mb(w, 2))
                    goto error;

                w->dest[1] += 8;
                w->dest[2] += 8;
            }
            w->dest[0] += 8;
        }
    }

error:
    // Partial decodes keep whatever was reconstructed (mirrors FFmpeg, which
    // returns 0 from the error path too).
    return 0;
}
