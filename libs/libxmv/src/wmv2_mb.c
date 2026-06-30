//------------------------------------------------------------------------------
// wmv2_mb.c -- WMV2 P-frame macroblock loop: motion-vector prediction + decode,
// half-pel motion compensation, and inter/intra residual (reusing the leak RL
// coding-set tables g_Inter/IntraDecoderTables + DC huffman tables). Ported from
// FFmpeg wmv2dec.c / msmpeg4dec.c / mpegvideo_motion.c onto the leak bit walker
// and frame planes. Increment 2 of the WMV2 P-frame port.
//
// Scope of this first cut: skip + inter macroblocks are fully reconstructed
// (MC + residual). Intra-in-P macroblocks are parsed for bit-sync and roughly
// reconstructed (spatial DC/AC prediction is a refinement, not yet applied).
// Per-block ABT (8x4/4x8) is parsed for sync; only the normal 8x8 transform is
// applied (abt_type 1/2 residual is skipped). Both are rare in practice.
//------------------------------------------------------------------------------

#include <xtl.h>
#include <xdbg.h>

#include "decoder.h"        // XmvVideoCore, bit walker, RL/DC tables, zigzag
#include "wmv2dec.h"
#include "wmv2_tables.h"

#define decode012(c) ((int)ReadTriStateBits(c))

// Diagnostic counters (see Wmv2DecodePFrame).
static int g_dbg_coded, g_dbg_abt;          // coded / ABT blocks this frame
static int g_dbg_pframes;                    // P-frames decoded so far (log gate)

// ---------------------------------------------------------------------------
// Inverse DCT (classic IEEE-1180 reference integer IDCT), producing raw spatial
// values; the caller clamps and either stores (intra) or adds (inter residual).
// ---------------------------------------------------------------------------
// Full 8x8 IDCT: the MPEG-reference IDCT (W1=2841..W7=565), which is exactly the
// transform the retail Xbox XMV decoder uses for keyframes (decoder/frontend.c
// InverseDCT: M1 = W7,W1-W7 = 565,2276). Matching it keeps the keyframe (leak)
// and P-frame paths consistent on real hardware. (FFmpeg decodes with a different,
// auto-selected IDCT, so an offline diff vs FFmpeg shows the IEEE-1180-permitted
// +-1 -- that is FFmpeg's IDCT differing, not a bug here.)
#define W1 2841
#define W2 2676
#define W3 2408
#define W5 1609
#define W6 1108
#define W7 565

static void idct_row(int *blk)
{
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

    if (!((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) |
          (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) {
        blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
        return;
    }
    x0 = (blk[0] << 11) + 128;
    x8 = W7 * (x4 + x5);  x4 = x8 + (W1 - W7) * x4;  x5 = x8 - (W1 + W7) * x5;
    x8 = W3 * (x6 + x7);  x6 = x8 - (W3 - W5) * x6;  x7 = x8 - (W3 + W5) * x7;
    x8 = x0 + x1;  x0 -= x1;  x1 = W6 * (x3 + x2);  x2 = x1 - (W2 + W6) * x2;  x3 = x1 + (W2 - W6) * x3;
    x1 = x4 + x6;  x4 -= x6;  x6 = x5 + x7;  x5 -= x7;
    x7 = x8 + x3;  x8 -= x3;  x3 = x0 + x2;  x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;  x4 = (181 * (x4 - x5) + 128) >> 8;
    blk[0] = (x7 + x1) >> 8;  blk[1] = (x3 + x2) >> 8;  blk[2] = (x0 + x4) >> 8;  blk[3] = (x8 + x6) >> 8;
    blk[4] = (x8 - x6) >> 8;  blk[5] = (x0 - x4) >> 8;  blk[6] = (x3 - x2) >> 8;  blk[7] = (x7 - x1) >> 8;
}

static void idct_col(int *blk)
{
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

    x1 = blk[8 * 4] << 8;  x2 = blk[8 * 6];  x3 = blk[8 * 2];
    x4 = blk[8 * 1];       x5 = blk[8 * 7];  x6 = blk[8 * 5];  x7 = blk[8 * 3];
    if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
        int v = (blk[0] + 32) >> 6;
        blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]=v;
        return;
    }
    x0 = (blk[0] << 8) + 8192;
    x8 = W7 * (x4 + x5) + 4;  x4 = (x8 + (W1 - W7) * x4) >> 3;  x5 = (x8 - (W1 + W7) * x5) >> 3;
    x8 = W3 * (x6 + x7) + 4;  x6 = (x8 - (W3 - W5) * x6) >> 3;  x7 = (x8 - (W3 + W5) * x7) >> 3;
    x8 = x0 + x1;  x0 -= x1;  x1 = W6 * (x3 + x2) + 4;  x2 = (x1 - (W2 + W6) * x2) >> 3;  x3 = (x1 + (W2 - W6) * x3) >> 3;
    x1 = x4 + x6;  x4 -= x6;  x6 = x5 + x7;  x5 -= x7;
    x7 = x8 + x3;  x8 -= x3;  x3 = x0 + x2;  x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;  x4 = (181 * (x4 - x5) + 128) >> 8;
    blk[8*0] = (x7 + x1) >> 14;  blk[8*1] = (x3 + x2) >> 14;  blk[8*2] = (x0 + x4) >> 14;  blk[8*3] = (x8 + x6) >> 14;
    blk[8*4] = (x8 - x6) >> 14;  blk[8*5] = (x0 - x4) >> 14;  blk[8*6] = (x3 - x2) >> 14;  blk[8*7] = (x7 - x1) >> 14;
}

static void idct_2d(int *blk)
{
    int i;
    for (i = 0; i < 8; i++) idct_row(blk + 8 * i);
    for (i = 0; i < 8; i++) idct_col(blk + i);
}

static BYTE clampU8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : (BYTE)v); }

static void idct_put(BYTE *dst, int stride, int *blk)
{
    int x, y;
    idct_2d(blk);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            dst[y * stride + x] = clampU8(blk[y * 8 + x]);
}

// ---------------------------------------------------------------------------
// WMV2 ABT (adaptive block transform) 8x4 / 4x8 IDCTs, ported from FFmpeg
// simple_idct.c (ff_simple_idct84_add / idct48_add + helpers). These use the
// FFmpeg simple_idct constants/scaling (distinct from the 8x8 idctref above);
// the 8-point and 4-point passes are designed to compose, so they must be used
// together for ABT blocks. The scan tables place coeffs into the 8x8 int block.
// ---------------------------------------------------------------------------
static const BYTE g_wmv2_scanA[64] = {
    0x00,0x01,0x02,0x08,0x03,0x09,0x0A,0x10, 0x04,0x0B,0x11,0x18,0x12,0x0C,0x05,0x13,
    0x19,0x0D,0x14,0x1A,0x1B,0x06,0x15,0x1C, 0x0E,0x16,0x1D,0x07,0x1E,0x0F,0x17,0x1F,
};
static const BYTE g_wmv2_scanB[64] = {
    0x00,0x08,0x01,0x10,0x09,0x18,0x11,0x02, 0x20,0x0A,0x19,0x28,0x12,0x30,0x21,0x1A,
    0x38,0x29,0x22,0x03,0x31,0x39,0x0B,0x2A, 0x13,0x32,0x1B,0x3A,0x23,0x2B,0x33,0x3B,
};

// Full-block WMV2 scans (ff_wmv1_scantable[0] inter, [1] intra). These are the
// "down-first" WMV2 zigzags and differ from the standard JPEG zigzag the leak
// hands us via g_NormalZigzag -- using the wrong one transposes the AC spectrum.
// We run an identity-permutation IDCT (idctref / simple_idct), so the RAW
// scantable maps scan position k -> blk[scan[k]] directly (no idct permute).
static const BYTE g_wmv2_inter_scan[64] = {
    0x00,0x08,0x01,0x02,0x09,0x10,0x18,0x11, 0x0A,0x03,0x04,0x0B,0x12,0x19,0x20,0x28,
    0x30,0x38,0x29,0x21,0x1A,0x13,0x0C,0x05, 0x06,0x0D,0x14,0x1B,0x22,0x31,0x39,0x3A,
    0x32,0x2A,0x23,0x1C,0x15,0x0E,0x07,0x0F, 0x16,0x1D,0x24,0x2B,0x33,0x3B,0x3C,0x34,
    0x2C,0x25,0x1E,0x17,0x1F,0x26,0x2D,0x35, 0x3D,0x3E,0x36,0x2E,0x27,0x2F,0x37,0x3F,
};
static const BYTE g_wmv2_intra_scan[64] = {
    0x00,0x08,0x01,0x02,0x09,0x10,0x18,0x11, 0x0A,0x03,0x04,0x0B,0x12,0x19,0x20,0x28,
    0x21,0x30,0x1A,0x13,0x0C,0x05,0x06,0x0D, 0x14,0x1B,0x22,0x29,0x38,0x31,0x39,0x2A,
    0x23,0x1C,0x15,0x0E,0x07,0x0F,0x16,0x1D, 0x24,0x2B,0x32,0x3A,0x33,0x3B,0x2C,0x25,
    0x1E,0x17,0x1F,0x26,0x2D,0x34,0x3C,0x35, 0x3D,0x2E,0x27,0x2F,0x36,0x3E,0x37,0x3F,
};
// ff_wmv1_scantable[2] (intra horizontal, used for ac_pred from top) and [3]
// (intra vertical, used for ac_pred from left).
static const BYTE g_wmv2_intra_h_scan[64] = {
    0x00,0x01,0x08,0x02,0x03,0x09,0x10,0x18, 0x11,0x0A,0x04,0x05,0x0B,0x12,0x19,0x20,
    0x28,0x30,0x21,0x1A,0x13,0x0C,0x06,0x07, 0x0D,0x14,0x1B,0x22,0x29,0x38,0x31,0x39,
    0x2A,0x23,0x1C,0x15,0x0E,0x0F,0x16,0x1D, 0x24,0x2B,0x32,0x3A,0x33,0x2C,0x25,0x1E,
    0x17,0x1F,0x26,0x2D,0x34,0x3B,0x3C,0x35, 0x2E,0x27,0x2F,0x36,0x3D,0x3E,0x37,0x3F,
};
static const BYTE g_wmv2_intra_v_scan[64] = {
    0x00,0x08,0x10,0x01,0x18,0x20,0x28,0x09, 0x02,0x03,0x0A,0x11,0x19,0x30,0x38,0x29,
    0x21,0x1A,0x12,0x0B,0x04,0x05,0x0C,0x13, 0x1B,0x22,0x31,0x39,0x32,0x2A,0x23,0x1C,
    0x14,0x0D,0x06,0x07,0x0E,0x15,0x1D,0x24, 0x2B,0x33,0x3A,0x3B,0x34,0x2C,0x25,0x1E,
    0x16,0x0F,0x17,0x1F,0x26,0x2D,0x3C,0x35, 0x2E,0x27,0x2F,0x36,0x3D,0x3E,0x37,0x3F,
};

#define SW1 22725
#define SW2 21407
#define SW3 19266
#define SW4 16383
#define SW5 12873
#define SW6 8867
#define SW7 4520
#define S_ROW_SHIFT 11
#define S_COL_SHIFT 20
// 4-point fixed-point constants (FFmpeg simple_idct.c).
#define R1 30274
#define R2 12540
#define R3 23170
#define R_SHIFT 11
#define C1 3784
#define C2 1567
#define C3 2896
#define C_SHIFT 17

// 8-point row IDCT (simple_idct idctRowCondDC), in place over row[0..7].
static void s_idct_row8(int *row)
{
    int a0, a1, a2, a3, b0, b1, b2, b3;
    a0 = SW4 * row[0] + (1 << (S_ROW_SHIFT - 1));
    a1 = a0; a2 = a0; a3 = a0;
    a0 += SW2 * row[2]; a1 += SW6 * row[2]; a2 -= SW6 * row[2]; a3 -= SW2 * row[2];
    b0 = SW1 * row[1] + SW3 * row[3];
    b1 = SW3 * row[1] - SW7 * row[3];
    b2 = SW5 * row[1] - SW1 * row[3];
    b3 = SW7 * row[1] - SW5 * row[3];
    a0 += SW4 * row[4] + SW6 * row[6]; a1 += -SW4 * row[4] - SW2 * row[6];
    a2 += -SW4 * row[4] + SW2 * row[6]; a3 += SW4 * row[4] - SW6 * row[6];
    b0 += SW5 * row[5] + SW7 * row[7];
    b1 += -SW1 * row[5] - SW5 * row[7];
    b2 += SW7 * row[5] + SW3 * row[7];
    b3 += SW3 * row[5] - SW1 * row[7];
    row[0] = (a0 + b0) >> S_ROW_SHIFT; row[7] = (a0 - b0) >> S_ROW_SHIFT;
    row[1] = (a1 + b1) >> S_ROW_SHIFT; row[6] = (a1 - b1) >> S_ROW_SHIFT;
    row[2] = (a2 + b2) >> S_ROW_SHIFT; row[5] = (a2 - b2) >> S_ROW_SHIFT;
    row[3] = (a3 + b3) >> S_ROW_SHIFT; row[4] = (a3 - b3) >> S_ROW_SHIFT;
}
// 8-point column IDCT add (simple_idct idctSparseColAdd); col stride 8.
static void s_idct_col8_add(BYTE *dest, int stride, int *col)
{
    int a0, a1, a2, a3, b0, b1, b2, b3;
    a0 = SW4 * (col[0] + ((1 << (S_COL_SHIFT - 1)) / SW4));
    a1 = a0; a2 = a0; a3 = a0;
    a0 += SW2 * col[16]; a1 += SW6 * col[16]; a2 -= SW6 * col[16]; a3 -= SW2 * col[16];
    b0 = SW1 * col[8]; b1 = SW3 * col[8]; b2 = SW5 * col[8]; b3 = SW7 * col[8];
    b0 += SW3 * col[24]; b1 -= SW7 * col[24]; b2 -= SW1 * col[24]; b3 -= SW5 * col[24];
    a0 += SW4 * col[32]; a1 -= SW4 * col[32]; a2 -= SW4 * col[32]; a3 += SW4 * col[32];
    b0 += SW5 * col[40]; b1 -= SW1 * col[40]; b2 += SW7 * col[40]; b3 += SW3 * col[40];
    a0 += SW6 * col[48]; a1 -= SW2 * col[48]; a2 += SW2 * col[48]; a3 -= SW6 * col[48];
    b0 += SW7 * col[56]; b1 -= SW5 * col[56]; b2 += SW3 * col[56]; b3 -= SW1 * col[56];
    dest[0]          = clampU8(dest[0]          + ((a0 + b0) >> S_COL_SHIFT));
    dest[stride]     = clampU8(dest[stride]     + ((a1 + b1) >> S_COL_SHIFT));
    dest[2 * stride] = clampU8(dest[2 * stride] + ((a2 + b2) >> S_COL_SHIFT));
    dest[3 * stride] = clampU8(dest[3 * stride] + ((a3 + b3) >> S_COL_SHIFT));
    dest[4 * stride] = clampU8(dest[4 * stride] + ((a3 - b3) >> S_COL_SHIFT));
    dest[5 * stride] = clampU8(dest[5 * stride] + ((a2 - b2) >> S_COL_SHIFT));
    dest[6 * stride] = clampU8(dest[6 * stride] + ((a1 - b1) >> S_COL_SHIFT));
    dest[7 * stride] = clampU8(dest[7 * stride] + ((a0 - b0) >> S_COL_SHIFT));
}
// 4-point row IDCT (simple_idct idct4row), in place over row[0..3].
static void s_idct4row(int *row)
{
    int c0, c1, c2, c3, a0 = row[0], a1 = row[1], a2 = row[2], a3 = row[3];
    c0 = (a0 + a2) * R3 + (1 << (R_SHIFT - 1));
    c2 = (a0 - a2) * R3 + (1 << (R_SHIFT - 1));
    c1 = a1 * R1 + a3 * R2;
    c3 = a1 * R2 - a3 * R1;
    row[0] = (c0 + c1) >> R_SHIFT; row[1] = (c2 + c3) >> R_SHIFT;
    row[2] = (c2 - c3) >> R_SHIFT; row[3] = (c0 - c1) >> R_SHIFT;
}
// 4-point column IDCT add (simple_idct idct4col_add); col stride 8.
static void s_idct4col_add(BYTE *dest, int stride, int *col)
{
    int c0, c1, c2, c3, a0 = col[0], a1 = col[8], a2 = col[16], a3 = col[24];
    c0 = (a0 + a2) * C3 + (1 << (C_SHIFT - 1));
    c2 = (a0 - a2) * C3 + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    dest[0]          = clampU8(dest[0]          + ((c0 + c1) >> C_SHIFT));
    dest[stride]     = clampU8(dest[stride]     + ((c2 + c3) >> C_SHIFT));
    dest[2 * stride] = clampU8(dest[2 * stride] + ((c2 - c3) >> C_SHIFT));
    dest[3 * stride] = clampU8(dest[3 * stride] + ((c0 - c1) >> C_SHIFT));
}
static void idct84_add(BYTE *dst, int stride, int *block)
{
    int i;
    for (i = 0; i < 4; i++) s_idct_row8(block + i * 8);
    for (i = 0; i < 8; i++) s_idct4col_add(dst + i, stride, block + i);
}
static void idct48_add(BYTE *dst, int stride, int *block)
{
    int i;
    for (i = 0; i < 8; i++) s_idct4row(block + i * 8);
    for (i = 0; i < 4; i++) s_idct_col8_add(dst + i, stride, block + i);
}

static void idct_add(BYTE *dst, int stride, int *blk)
{
    int x, y;
    idct_2d(blk);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            dst[y * stride + x] = clampU8(dst[y * stride + x] + blk[y * 8 + x]);
}

// ---------------------------------------------------------------------------
// Half-pel motion compensation with edge clamping (no padded reference planes).
// dxy bit0 = half-pel x, bit1 = half-pel y. round = 1 for normal rounding.
// ---------------------------------------------------------------------------
static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void mc_block(BYTE *dst, int dstStride, const BYTE *ref, int refStride,
                     int refW, int refH, int bw, int bh,
                     int srcX, int srcY, int dxy, int round)
{
    int x, y;
    int add4 = round ? 2 : 1;
    int add2 = round ? 1 : 0;

    // Fast path: the whole footprint (incl. +1 half-pel neighbour) is inside.
    if (srcX >= 0 && srcY >= 0 && srcX + bw + 1 <= refW && srcY + bh + 1 <= refH) {
        const BYTE *s = ref + srcY * refStride + srcX;
        for (y = 0; y < bh; y++) {
            const BYTE *r = s + y * refStride;
            BYTE *d = dst + y * dstStride;
            switch (dxy) {
            case 0: for (x = 0; x < bw; x++) d[x] = r[x]; break;
            case 1: for (x = 0; x < bw; x++) d[x] = (BYTE)((r[x] + r[x + 1] + add2) >> 1); break;
            case 2: for (x = 0; x < bw; x++) d[x] = (BYTE)((r[x] + r[x + refStride] + add2) >> 1); break;
            default:for (x = 0; x < bw; x++)
                        d[x] = (BYTE)((r[x] + r[x + 1] + r[x + refStride] + r[x + refStride + 1] + add4) >> 2);
                break;
            }
        }
        return;
    }

    // Slow path: clamp every sample to the plane.
    for (y = 0; y < bh; y++) {
        for (x = 0; x < bw; x++) {
            int sx = srcX + x, sy = srcY + y;
            int p00 = ref[clampi(sy, 0, refH - 1) * refStride + clampi(sx, 0, refW - 1)];
            int v;
            if (dxy == 0) {
                v = p00;
            } else if (dxy == 1) {
                int p10 = ref[clampi(sy, 0, refH - 1) * refStride + clampi(sx + 1, 0, refW - 1)];
                v = (p00 + p10 + add2) >> 1;
            } else if (dxy == 2) {
                int p01 = ref[clampi(sy + 1, 0, refH - 1) * refStride + clampi(sx, 0, refW - 1)];
                v = (p00 + p01 + add2) >> 1;
            } else {
                int p10 = ref[clampi(sy, 0, refH - 1) * refStride + clampi(sx + 1, 0, refW - 1)];
                int p01 = ref[clampi(sy + 1, 0, refH - 1) * refStride + clampi(sx, 0, refW - 1)];
                int p11 = ref[clampi(sy + 1, 0, refH - 1) * refStride + clampi(sx + 1, 0, refW - 1)];
                v = (p00 + p10 + p01 + p11 + add4) >> 2;
            }
            dst[y * dstStride + x] = (BYTE)v;
        }
    }
}

// ---------------------------------------------------------------------------
// WMV2 mspel (Microsoft sub-pel) 9-tap interpolation, used for luma motion comp
// when the frame's mspel flag is set. Ported from FFmpeg wmv2dec.c
// (wmv2_mspel8_*_lowpass + put_mspel8_mc* + ff_mspel_motion). Chroma stays
// half-pel (mc_block) but with the mspel chroma-MV derivation.
// ---------------------------------------------------------------------------
static void mspel_hlp(BYTE *dst, const BYTE *src, int dS, int sS, int h)
{
    int i, x;
    for (i = 0; i < h; i++) {
        for (x = 0; x < 8; x++)
            dst[x] = clampU8((9 * (src[x] + src[x + 1]) - (src[x - 1] + src[x + 2]) + 8) >> 4);
        dst += dS; src += sS;
    }
}
static void mspel_vlp(BYTE *dst, const BYTE *src, int dS, int sS, int w)
{
    int i, y;
    for (i = 0; i < w; i++) {
        for (y = 0; y < 8; y++)
            dst[y * dS] = clampU8((9 * (src[y * sS] + src[(y + 1) * sS]) -
                                   (src[(y - 1) * sS] + src[(y + 2) * sS]) + 8) >> 4);
        src++; dst++;
    }
}
static void avg8(BYTE *dst, const BYTE *a, const BYTE *b, int dS, int aS, int bS)
{
    int j, x;
    for (j = 0; j < 8; j++) {
        for (x = 0; x < 8; x++) dst[x] = (BYTE)((a[x] + b[x] + 1) >> 1);
        dst += dS; a += aS; b += bS;
    }
}
static void copy8(BYTE *dst, const BYTE *src, int dS, int sS)
{
    int j, x;
    for (j = 0; j < 8; j++) { for (x = 0; x < 8; x++) dst[x] = src[x]; dst += dS; src += sS; }
}
// One 8x8 luma block; src points into a padded buffer (stride sS); dxy 0..7.
static void mspel_put8(BYTE *dst, int dS, const BYTE *src, int sS, int dxy)
{
    BYTE half[64], halfH[11 * 8], halfV[64], halfHV[64];
    switch (dxy) {
    case 0: copy8(dst, src, dS, sS); break;
    case 1: mspel_hlp(half, src, 8, sS, 8); avg8(dst, src, half, dS, sS, 8); break;
    case 2: mspel_hlp(dst, src, dS, sS, 8); break;
    case 3: mspel_hlp(half, src, 8, sS, 8); avg8(dst, src + 1, half, dS, sS, 8); break;
    case 4: mspel_vlp(dst, src, dS, sS, 8); break;
    case 5: mspel_hlp(halfH, src - sS, 8, sS, 11); mspel_vlp(halfV, src, 8, sS, 8);
            mspel_vlp(halfHV, halfH + 8, 8, 8, 8); avg8(dst, halfV, halfHV, dS, 8, 8); break;
    case 6: mspel_hlp(halfH, src - sS, 8, sS, 11); mspel_vlp(dst, halfH + 8, dS, 8, 8); break;
    default:mspel_hlp(halfH, src - sS, 8, sS, 11); mspel_vlp(halfV, src + 1, 8, sS, 8);
            mspel_vlp(halfHV, halfH + 8, 8, 8, 8); avg8(dst, halfV, halfHV, dS, 8, 8); break;
    }
}
// Full mspel motion for one macroblock: 9-tap luma + half-pel chroma.
static void wmv2_mspel_motion(Wmv2 *w, BYTE *Ydst, BYTE *Udst, BYTE *Vdst,
                              int Ypitch, int UVpitch, int mb_x, int mb_y,
                              int mx, int my, int hshift, int round)
{
    XmvVideoCore *c = w->core;
    int dxy = 2 * (((my & 1) << 1) | (mx & 1)) + hshift;
    int src_x = mb_x * 16 + (mx >> 1);
    int src_y = mb_y * 16 + (my >> 1);
    int W = (int)c->Width, H = (int)c->Height;
    const BYTE *ref = c->pYDisplayed;
    BYTE buf[24 * 24];
    const BYTE *ptr;
    int i, j, cmx, cmy, cdxy, csx, csy;

    // Extract a 22x22 padded region around (src_x-1, src_y-1) with edge clamp.
    for (j = 0; j < 22; j++) {
        int sy = clampi(src_y - 1 + j, 0, H - 1);
        for (i = 0; i < 22; i++) {
            int sx = clampi(src_x - 1 + i, 0, W - 1);
            buf[j * 24 + i] = ref[sy * W + sx];
        }
    }
    ptr = buf + 24 + 1;   // buf[1][1] == ref(src_x, src_y)
    mspel_put8(Ydst,                       Ypitch, ptr,                24, dxy);
    mspel_put8(Ydst + 8,                   Ypitch, ptr + 8,            24, dxy);
    mspel_put8(Ydst + 8 * Ypitch,          Ypitch, ptr + 8 * 24,       24, dxy);
    mspel_put8(Ydst + 8 + 8 * Ypitch,      Ypitch, ptr + 8 + 8 * 24,   24, dxy);

    // Chroma: mspel derivation (motion>>2, dxy from motion&3), half-pel.
    cdxy = ((my & 3) ? 2 : 0) | ((mx & 3) ? 1 : 0);
    cmx  = mx >> 2;
    cmy  = my >> 2;
    csx  = mb_x * 8 + cmx;
    csy  = mb_y * 8 + cmy;
    mc_block(Udst, UVpitch, c->pUDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
             8, 8, csx, csy, cdxy, round);
    mc_block(Vdst, UVpitch, c->pVDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
             8, 8, csx, csy, cdxy, round);
}

// ---------------------------------------------------------------------------
// Run/level AC decode using a leak coding-set table. Mirrors the leak's
// DecodeBaselineIFrameBlock AC loop (frontend.c) minus intra AC prediction:
// places dequantized coefficients into blk[64] (natural order) via the normal
// zigzag. `counter` is the starting scan position (0 inter, 1 intra-after-DC).
// ---------------------------------------------------------------------------
static void decode_ac(XmvVideoCore *c, Wmv2 *w, XMVACCoefficientDecoderTable *tbl,
                      int *blk, int counter, int qscale, const BYTE *scan)
{
    int D2   = qscale * 2;
    int Qodd = (qscale & 1) ? qscale : qscale - 1;
    int Done, Run;
    long Level;
    DWORD Index;

    for (;;) {
        Index = HuffmanDecode(c, tbl->pDTCACDecoderTable);

        if (Index != tbl->DCTACDecoderEscapeCode) {
            Done  = (Index >= tbl->StartIndexOfLastRun);
            Run   = tbl->RunTable[Index];
            Level = tbl->LevelTable[Index];
            if (ReadOneBit(c)) Level = -Level;
        } else if (ReadOneBit(c)) {                                    // ESC + 1
            Index = HuffmanDecode(c, tbl->pDTCACDecoderTable);
            Done  = (Index >= tbl->StartIndexOfLastRun);
            Run   = tbl->RunTable[Index];
            Level = tbl->LevelTable[Index];
            Level += Done ? tbl->LastDeltaLevelTable[Run] : tbl->NotLastDeltaLevelTable[Run];
            if (ReadOneBit(c)) Level = -Level;
        } else if (ReadOneBit(c)) {                                    // ESC + 01
            Index = HuffmanDecode(c, tbl->pDTCACDecoderTable);
            Done  = (Index >= tbl->StartIndexOfLastRun);
            Run   = tbl->RunTable[Index];
            Level = tbl->LevelTable[Index];
            Run += (Done ? tbl->LastDeltaRunTable[Level] : tbl->NotLastDeltaRunTable[Level]) + 1;
            if (ReadOneBit(c)) Level = -Level;
        } else {                                                       // ESC + 00 (esc3)
            int Sign;
            Done = (int)ReadOneBit(c);
            if (!w->esc3_run_length) {
                if (qscale >= 8) {
                    int i = 0, Bit = 0;
                    while (i < 6 && !Bit) { Bit = (int)ReadOneBit(c); i++; }
                    w->esc3_level_length = Bit ? i + 1 : 8;
                } else {
                    w->esc3_level_length = (int)ReadBits(c, 3);
                    if (!w->esc3_level_length)
                        w->esc3_level_length = 8 + (int)ReadOneBit(c);
                }
                w->esc3_run_length = 3 + (int)ReadBits(c, 2);
            }
            Run   = (int)ReadBits(c, w->esc3_run_length);
            Sign  = (int)ReadOneBit(c);
            Level = (long)ReadBits(c, w->esc3_level_length);
            if (Sign) Level = -Level;
        }

        counter += Run;
        if (counter > 63) counter = 63;
        {
            int idx = scan[counter];
            blk[idx] = (Level > 0) ? (D2 * (int)Level + Qodd) : (D2 * (int)Level - Qodd);
        }
        counter++;
        if (Done) break;
    }
}

// Decode + reconstruct one inter block (residual) into dest via IDCT-add.
static void decode_inter_block(XmvVideoCore *c, Wmv2 *w, int coded, int rl_index,
                               BYTE *dest, int stride)
{
    int blk[64];
    if (!coded) return;
    memset(blk, 0, sizeof(blk));
    decode_ac(c, w, &g_InterDecoderTables[rl_index], blk, 0, w->qscale, g_wmv2_inter_scan);
    idct_add(dest, stride, blk);
}

// WMV1/WMV2 DC scale tables (ff_wmv1_{y,c}_dc_scale_table), indexed by qscale.
static const BYTE g_wmv2_y_dc_scale[32] = {
     0,  8,  8,  8,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13,
    14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
};
static const BYTE g_wmv2_c_dc_scale[32] = {
     0,  8,  8,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14,
    14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22,
};

// Decode + reconstruct one intra block (DC huffman + AC) into dest via IDCT-put.
// Implements the msmpeg4/WMV2 spatial DC prediction (ff_msmpeg4_pred_dc): the DC
// is a delta from the left- or top-neighbour block's DC (chosen by gradient),
// read from the per-frame w->dc_{y,u,v} grids (reset to 1024, so inter/skip
// neighbours predict the default DC). ac_pred is parsed by the caller; AC spatial
// prediction is not applied (streams here use ac_pred=0).
static void decode_intra_block_p(XmvVideoCore *c, Wmv2 *w, int mb_x, int mb_y,
                                 int n, int coded, int ac_pred, BYTE *dest, int stride)
{
    int blk[64];
    int scale = (n < 4) ? g_wmv2_y_dc_scale[w->qscale] : g_wmv2_c_dc_scale[w->qscale];
    int16_t *grid, *acg, *acself;
    int gstride, gidx, a, b, cc, pa, pb, pc, pred, dir, i;
    WORD *dctable;
    DWORD mag;
    int dc;

    memset(blk, 0, sizeof(blk));

    // Locate this block in its DC/AC grids (luma b8 resolution, chroma MB res).
    if (n < 4) {
        grid = w->dc_y; acg = w->ac_y; gstride = w->dc_y_stride;
        gidx = (mb_y * 2 + 1 + (n >> 1)) * gstride + (mb_x * 2 + 1 + (n & 1));
    } else {
        grid = (n == 4) ? w->dc_u : w->dc_v;
        acg  = (n == 4) ? w->ac_u : w->ac_v; gstride = w->dc_c_stride;
        gidx = (mb_y + 1) * gstride + (mb_x + 1);
    }
    acself = acg + (size_t)gidx * 16;

    // Gradient DC prediction from left (a), top-left (b), top (cc); dir picks the
    // predictor (0 = left, 1 = top) and also selects the AC-prediction neighbour.
    a  = grid[gidx - 1];
    b  = grid[gidx - 1 - gstride];
    cc = grid[gidx - gstride];
    pa = (a  + (scale >> 1)) / scale;
    pb = (b  + (scale >> 1)) / scale;
    pc = (cc + (scale >> 1)) / scale;
    if (abs(pa - pb) < abs(pb - pc)) { pred = pc; dir = 1; }
    else                            { pred = pa; dir = 0; }

    if (n < 4)
        dctable = w->dc_table_index ? g_Huffman_DCTDCy_HighMotion : g_Huffman_DCTDCy_Talking;
    else
        dctable = w->dc_table_index ? g_Huffman_DCTDCc_HighMotion : g_Huffman_DCTDCc_Talking;

    mag = HuffmanDecode(c, dctable);
    if (mag == INTRADCYTCOEF_ESCAPE_CODE)   // 119, same escape for luma/chroma
        mag = ReadBits(c, 8);
    if (mag && ReadOneBit(c))
        dc = pred - (int)mag;
    else
        dc = pred + (int)mag;

    blk[0]     = dc * scale;
    grid[gidx] = (int16_t)blk[0];

    if (coded) {
        int rl = (n < 4) ? w->rl_table_index : w->rl_chroma_table_index;
        XMVACCoefficientDecoderTable *tbl = (n < 4)
            ? &g_IntraDecoderTables[rl] : &g_InterDecoderTables[rl];
        // ac_pred selects the vertical (dir=left) or horizontal (dir=top) scan so
        // the predicted row/column lands first; otherwise the plain intra scan.
        const BYTE *scan = !ac_pred ? g_wmv2_intra_scan
                         : (dir == 0 ? g_wmv2_intra_v_scan : g_wmv2_intra_h_scan);
        decode_ac(c, w, tbl, blk, 1, w->qscale, scan);
    }

    // AC prediction (ff_mpeg4_pred_ac): add the neighbour's stored AC, then store
    // this block's left column / top row for future predictors. qscale is constant
    // per WMV2 frame, so no rescale path is needed.
    if (ac_pred) {
        if (dir == 0) {                                  // from left neighbour
            int16_t *acl = acg + (size_t)(gidx - 1) * 16;
            for (i = 1; i < 8; i++) blk[i << 3] += acl[i];
        } else {                                         // from top neighbour
            int16_t *act = acg + (size_t)(gidx - gstride) * 16;
            for (i = 1; i < 8; i++) blk[i] += act[8 + i];
        }
    }
    for (i = 1; i < 8; i++) acself[i]     = (int16_t)blk[i << 3];   // left column
    for (i = 1; i < 8; i++) acself[8 + i] = (int16_t)blk[i];        // top row

    idct_put(dest, stride, blk);
}

// ---------------------------------------------------------------------------
// Motion vector prediction (wmv2_pred_motion) + decode (ff_msmpeg4_decode_motion).
// ---------------------------------------------------------------------------
static int mid3(int a, int b, int c)
{
    int mn = a < b ? a : b, mx = a > b ? a : b;
    if (c < mn) return mn;
    if (c > mx) return mx;
    return c;
}

static void wmv2_pred_motion(Wmv2 *w, int mb_x, int mb_y, int first_slice_line,
                             int *px, int *py)
{
    int s   = w->mv_stride;
    int g   = (mb_y + 1) * s + (mb_x + 1);
    int Ax = w->mv_x[g - 1],     Ay = w->mv_y[g - 1];        // left
    int Bx = w->mv_x[g - s],     By = w->mv_y[g - s];        // top
    int Cx = w->mv_x[g - s + 1], Cy = w->mv_y[g - s + 1];    // top-right
    int diff, type;

    if (mb_x && !first_slice_line && !w->mspel && w->top_left_mv_flag) {
        int dx = Ax - Bx, dy = Ay - By;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        diff = dx > dy ? dx : dy;
    } else {
        diff = 0;
    }

    if (diff >= 8)
        type = (int)ReadOneBit(w->core);
    else
        type = 2;

    if (type == 0) {
        *px = Ax; *py = Ay;
    } else if (type == 1) {
        *px = Bx; *py = By;
    } else {
        if (first_slice_line) { *px = Ax; *py = Ay; }
        else { *px = mid3(Ax, Bx, Cx); *py = mid3(Ay, By, Cy); }
    }
}

static void wmv2_decode_motion(Wmv2 *w, int px, int py, int *mx_out, int *my_out)
{
    XmvVideoCore *c = w->core;
    int sym = Wmv2VlcDecode(c, &w->mv_vlc[w->mv_table_index]);
    int mx, my;

    if (sym) {
        mx = sym >> 8;
        my = sym & 0xFF;
    } else {
        mx = (int)ReadBits(c, 6);
        my = (int)ReadBits(c, 6);
    }
    mx += px - 32;
    my += py - 32;
    if (mx <= -64) mx += 64; else if (mx >= 64) mx -= 64;
    if (my <= -64) my += 64; else if (my >= 64) my -= 64;

    // mspel: an extra "half-pel shift" bit when the MV is half-pel and mspel is on.
    if (((mx | my) & 1) && w->mspel)
        w->hshift = (int)ReadOneBit(c);
    else
        w->hshift = 0;

    *mx_out = mx;
    *my_out = my;
}

void Wmv2ResetMotion(Wmv2 *w)
{
    size_t n = (size_t)w->mv_stride * (w->core->MBHeight + 1);
    memset(w->mv_x, 0, n * sizeof(int16_t));
    memset(w->mv_y, 0, n * sizeof(int16_t));
}

// ---------------------------------------------------------------------------
// Whole-frame P decode. Bit walker must be at the start of the frame; the
// primary + secondary headers have already consumed their bits. `frame`/`size`
// are only for the diagnostic bit-consumption check.
// ---------------------------------------------------------------------------
int Wmv2DecodePFrame(Wmv2 *w, const unsigned char *frame, unsigned size)
{
    XmvVideoCore *c = w->core;
    int mbw = (int)c->MBWidth, mbh = (int)c->MBHeight;
    int Ypitch = mbw * 16, UVpitch = mbw * 8;
    int round = !w->no_rounding;
    int slice_h = (w->slice_code > 0) ? (mbh / w->slice_code) : mbh;
    int mb_x, mb_y, n;
    int n_skip = 0, n_inter = 0, n_intra = 0;

    g_dbg_coded = 0;
    g_dbg_abt   = 0;
    Wmv2ResetMotion(w);

    // Reset the intra DC-prediction grids: 1024 everywhere (= default DC), so
    // inter/skip neighbours predict the default and only intra blocks override.
    {
        int i, ny = w->dc_y_stride * (mbh * 2 + 2), nc = w->dc_c_stride * (mbh + 2);
        for (i = 0; i < ny; i++) w->dc_y[i] = 1024;
        for (i = 0; i < nc; i++) w->dc_u[i] = w->dc_v[i] = 1024;
        memset(w->ac_y, 0, (size_t)ny * 16 * sizeof(int16_t));
        memset(w->ac_u, 0, (size_t)nc * 16 * sizeof(int16_t));
        memset(w->ac_v, 0, (size_t)nc * 16 * sizeof(int16_t));
    }

    for (mb_y = 0; mb_y < mbh; mb_y++) {
        int first_slice_line = (slice_h > 0) ? ((mb_y % slice_h) == 0) : (mb_y == 0);

        for (mb_x = 0; mb_x < mbw; mb_x++) {
            int idx = mb_y * mbw + mb_x;
            int g   = (mb_y + 1) * w->mv_stride + (mb_x + 1);
            BYTE *Ydst = c->pYBuilding + mb_y * 16 * Ypitch + mb_x * 16;
            BYTE *Udst = c->pUBuilding + mb_y * 8 * UVpitch + mb_x * 8;
            BYTE *Vdst = c->pVBuilding + mb_y * 8 * UVpitch + mb_x * 8;
            int code, mb_intra, cbp;

            if (w->mb_skip[idx]) {
                // Skipped: zero MV, copy co-located block from the reference.
                n_skip++;
                w->mv_x[g] = 0; w->mv_y[g] = 0;
                mc_block(Ydst, Ypitch, c->pYDisplayed, Ypitch, (int)c->Width, (int)c->Height,
                         16, 16, mb_x * 16, mb_y * 16, 0, round);
                mc_block(Udst, UVpitch, c->pUDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
                         8, 8, mb_x * 8, mb_y * 8, 0, round);
                mc_block(Vdst, UVpitch, c->pVDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
                         8, 8, mb_x * 8, mb_y * 8, 0, round);
                continue;
            }

            code     = Wmv2VlcDecode(c, &w->cbp_vlc[w->cbp_table_index]);
            mb_intra = (~code & 0x40) >> 6;
            cbp      = code & 0x3f;

            if (!mb_intra) {
                int px, py, mx, my, dxy, uvdxy, src_x, src_y, uvsx, uvsy;

                n_inter++;
                wmv2_pred_motion(w, mb_x, mb_y, first_slice_line, &px, &py);

                if (cbp) {
                    if (w->per_mb_rl_table) {
                        w->rl_table_index        = decode012(c);
                        w->rl_chroma_table_index = w->rl_table_index;
                    }
                    if (w->abt_flag && w->per_mb_abt) {
                        w->per_block_abt = (int)ReadOneBit(c);
                        if (!w->per_block_abt)
                            w->abt_type = decode012(c);
                    } else {
                        w->per_block_abt = 0;
                    }
                }

                wmv2_decode_motion(w, px, py, &mx, &my);
                w->mv_x[g] = (int16_t)mx;
                w->mv_y[g] = (int16_t)my;

                if (w->mspel) {
                    wmv2_mspel_motion(w, Ydst, Udst, Vdst, Ypitch, UVpitch,
                                      mb_x, mb_y, mx, my, w->hshift, round);
                } else {
                    dxy   = ((my & 1) << 1) | (mx & 1);
                    src_x = mb_x * 16 + (mx >> 1);
                    src_y = mb_y * 16 + (my >> 1);
                    uvsx  = src_x >> 1;
                    uvsy  = src_y >> 1;
                    uvdxy = dxy | (my & 2) | ((mx & 2) >> 1);

                    mc_block(Ydst, Ypitch, c->pYDisplayed, Ypitch, (int)c->Width, (int)c->Height,
                             16, 16, src_x, src_y, dxy, round);
                    mc_block(Udst, UVpitch, c->pUDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
                             8, 8, uvsx, uvsy, uvdxy, round);
                    mc_block(Vdst, UVpitch, c->pVDisplayed, UVpitch, (int)c->UVWidth, (int)c->UVHeight,
                             8, 8, uvsx, uvsy, uvdxy, round);
                }

                // Inter residual. abt_type per coded block = the per-block value
                // (per_block_abt) or the MB-level value; abt_type != 0 selects an
                // 8x4 (type 1) or 4x8 (type 2) adaptive block transform.
                for (n = 0; n < 6; n++) {
                    int coded = (cbp >> (5 - n)) & 1;
                    int abt_t;
                    BYTE *bd;
                    int   bstride;

                    if (!coded)
                        continue;
                    g_dbg_coded++;

                    switch (n) {
                    case 0: bd = Ydst;                  bstride = Ypitch;  break;
                    case 1: bd = Ydst + 8;              bstride = Ypitch;  break;
                    case 2: bd = Ydst + 8 * Ypitch;     bstride = Ypitch;  break;
                    case 3: bd = Ydst + 8 + 8 * Ypitch; bstride = Ypitch;  break;
                    case 4: bd = Udst;                  bstride = UVpitch; break;
                    default:bd = Vdst;                  bstride = UVpitch; break;
                    }

                    abt_t = w->per_block_abt ? decode012(c) : w->abt_type;

                    if (abt_t) {
                        static const int sub_cbp_table[3] = { 2, 3, 1 };
                        const BYTE *scan = (abt_t == 1) ? g_wmv2_scanA : g_wmv2_scanB;
                        int sub = sub_cbp_table[decode012(c)];
                        int blk1[64], blk2[64];
                        memset(blk1, 0, sizeof(blk1));
                        memset(blk2, 0, sizeof(blk2));
                        if (sub & 1)
                            decode_ac(c, w, &g_InterDecoderTables[w->rl_table_index], blk1, 0, w->qscale, scan);
                        if (sub & 2)
                            decode_ac(c, w, &g_InterDecoderTables[w->rl_table_index], blk2, 0, w->qscale, scan);
                        if (abt_t == 1) {            // two stacked 8x4 transforms
                            idct84_add(bd, bstride, blk1);
                            idct84_add(bd + 4 * bstride, bstride, blk2);
                        } else {                     // two side-by-side 4x8 transforms
                            idct48_add(bd, bstride, blk1);
                            idct48_add(bd + 4, bstride, blk2);
                        }
                        g_dbg_abt++;
                    } else {
                        decode_inter_block(c, w, 1, w->rl_table_index, bd, bstride);
                    }
                }
            } else {
                // Intra macroblock inside a P-frame.
                int ac_pred;
                n_intra++;
                w->mv_x[g] = 0; w->mv_y[g] = 0;

                ac_pred = (int)ReadOneBit(c);

                if (w->per_mb_rl_table && cbp) {
                    w->rl_table_index        = decode012(c);
                    w->rl_chroma_table_index = w->rl_table_index;
                }

                for (n = 0; n < 6; n++) {
                    int coded = (cbp >> (5 - n)) & 1;
                    BYTE *bd;
                    int   bstride;
                    switch (n) {
                    case 0: bd = Ydst;                  bstride = Ypitch;  break;
                    case 1: bd = Ydst + 8;              bstride = Ypitch;  break;
                    case 2: bd = Ydst + 8 * Ypitch;     bstride = Ypitch;  break;
                    case 3: bd = Ydst + 8 + 8 * Ypitch; bstride = Ypitch;  break;
                    case 4: bd = Udst;                  bstride = UVpitch; break;
                    default:bd = Vdst;                  bstride = UVpitch; break;
                    }
                    decode_intra_block_p(c, w, mb_x, mb_y, n, coded, ac_pred, bd, bstride);
                }
            }
        }
    }

    // First-frame sanity: MB composition + bit consumption vs frame size.
    // consumed ~= size (within a few bytes of bit-reader lookahead) means the
    // parse stayed in sync end-to-end.
    if (g_dbg_pframes == 0) {
        int consumed = (int)(c->pDecodingPosition - (BYTE *)frame);
        DbgPrint("wmv2: P-frame skip=%d inter=%d intra=%d | coded=%d abt=%d | consumed=%d/%u\n",
                 n_skip, n_inter, n_intra, g_dbg_coded, g_dbg_abt, consumed, size);
    }
    g_dbg_pframes++;

    return 0;
}
