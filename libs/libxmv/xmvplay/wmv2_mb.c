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
static int g_dbg_ac;                         // AC coefficient decodes logged
static const BYTE *g_fstart;                 // frame start (for bit-position logging)
static int g_dbg_mb;                         // trace the current MB (set for mb(0,12))
#define BITPOS(c) ((int)(((c)->pDecodingPosition - g_fstart) * 8 - (int)(c)->BitsRemaining))

// ---------------------------------------------------------------------------
// Inverse DCT (classic IEEE-1180 reference integer IDCT), producing raw spatial
// values; the caller clamps and either stores (intra) or adds (inter residual).
// ---------------------------------------------------------------------------
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
// Run/level AC decode using a leak coding-set table. Mirrors the leak's
// DecodeBaselineIFrameBlock AC loop (frontend.c) minus intra AC prediction:
// places dequantized coefficients into blk[64] (natural order) via the normal
// zigzag. `counter` is the starting scan position (0 inter, 1 intra-after-DC).
// ---------------------------------------------------------------------------
static void decode_ac(XmvVideoCore *c, Wmv2 *w, XMVACCoefficientDecoderTable *tbl,
                      int *blk, int counter, int qscale)
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

        if (g_dbg_mb) {
            DbgPrint("wmv2:       ac idx=%u run=%d lvl=%d done=%d bitpos=%d\n",
                     (unsigned)Index, Run, (int)Level, Done, BITPOS(w->core));
        }
        counter += Run;
        if (counter > 63) counter = 63;
        {
            int idx = g_NormalZigzag[counter];
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
    decode_ac(c, w, &g_InterDecoderTables[rl_index], blk, 0, w->qscale);
    idct_add(dest, stride, blk);
}

// Decode + reconstruct one intra block (DC huffman + AC) into dest via IDCT-put.
// First cut: no spatial DC/AC prediction (rare in P-frames); maintains bit-sync.
static void decode_intra_block_p(XmvVideoCore *c, Wmv2 *w, int n, int coded,
                                 BYTE *dest, int stride)
{
    int blk[64];
    int DCStepSize  = (w->qscale <= 4) ? 8 : (w->qscale / 2 + 6);
    int defaultDC   = (1024 + (DCStepSize >> 1)) / DCStepSize;
    WORD *dctable;
    DWORD mag;
    int dc;

    memset(blk, 0, sizeof(blk));

    if (n < 4)
        dctable = w->dc_table_index ? g_Huffman_DCTDCy_HighMotion : g_Huffman_DCTDCy_Talking;
    else
        dctable = w->dc_table_index ? g_Huffman_DCTDCc_HighMotion : g_Huffman_DCTDCc_Talking;

    mag = HuffmanDecode(c, dctable);
    if (mag == INTRADCYTCOEF_ESCAPE_CODE)   // 119, same escape for luma/chroma
        mag = ReadBits(c, 8);
    if (mag && ReadOneBit(c))
        dc = defaultDC - (int)mag;
    else
        dc = defaultDC + (int)mag;

    blk[0] = dc * DCStepSize;

    if (coded) {
        int rl = (n < 4) ? w->rl_table_index : w->rl_chroma_table_index;
        XMVACCoefficientDecoderTable *tbl = (n < 4)
            ? &g_IntraDecoderTables[rl] : &g_InterDecoderTables[rl];
        decode_ac(c, w, tbl, blk, 1, w->qscale);
    }

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
    // mspel == 0 for our stream, so no hshift bit.
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
    g_fstart    = (const BYTE *)frame;
    Wmv2ResetMotion(w);

    if (g_dbg_pframes == 0)
        DbgPrint("wmv2:   after-hdr consumed=%d bytes (skip bitmap cost)\n",
                 (int)(c->pDecodingPosition - (BYTE *)frame));

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

            g_dbg_mb = (g_dbg_pframes == 0 && mb_x == 0 && mb_y == 12);
            if (g_dbg_mb) DbgPrint("wmv2:   mb(0,12) bit before MB-type=%d\n", BITPOS(c));

            code     = Wmv2VlcDecode(c, &w->cbp_vlc[w->cbp_table_index]);
            mb_intra = (~code & 0x40) >> 6;
            cbp      = code & 0x3f;

            if (g_dbg_mb) DbgPrint("wmv2:   mb(0,12) code=%d cbp=0x%02x bit after MB-type=%d\n",
                                   code, cbp, BITPOS(c));

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

                if (g_dbg_mb) DbgPrint("wmv2:   mb(0,12) mv=(%d,%d) pred=(%d,%d) bit after MV=%d\n",
                                       mx, my, px, py, BITPOS(c));

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

                // Inter residual. abt_type per coded block = the per-block value
                // (per_block_abt) or the MB-level value; abt_type != 0 selects an
                // 8x4/4x8 transform -- parsed for bit-sync, residual not yet applied.
                for (n = 0; n < 6; n++) {
                    int coded = (cbp >> (5 - n)) & 1;
                    int abt_t;
                    BYTE *bd;
                    int   bstride;

                    if (!coded)
                        continue;
                    g_dbg_coded++;

                    abt_t = w->per_block_abt ? decode012(c) : w->abt_type;

                    if (abt_t) {
                        static const int sub_cbp_table[3] = { 2, 3, 1 };
                        int sub = sub_cbp_table[decode012(c)];
                        int scratch[64];
                        if (sub & 1) { memset(scratch,0,sizeof(scratch)); decode_ac(c,w,&g_InterDecoderTables[w->rl_table_index],scratch,0,w->qscale); }
                        if (sub & 2) { memset(scratch,0,sizeof(scratch)); decode_ac(c,w,&g_InterDecoderTables[w->rl_table_index],scratch,0,w->qscale); }
                        g_dbg_abt++;
                        continue;
                    }

                    switch (n) {
                    case 0: bd = Ydst;                         bstride = Ypitch;  break;
                    case 1: bd = Ydst + 8;                     bstride = Ypitch;  break;
                    case 2: bd = Ydst + 8 * Ypitch;            bstride = Ypitch;  break;
                    case 3: bd = Ydst + 8 + 8 * Ypitch;        bstride = Ypitch;  break;
                    case 4: bd = Udst;                         bstride = UVpitch; break;
                    default:bd = Vdst;                         bstride = UVpitch; break;
                    }
                    decode_inter_block(c, w, 1, w->rl_table_index, bd, bstride);
                }
            } else {
                // Intra macroblock inside a P-frame.
                int ac_pred;
                n_intra++;
                w->mv_x[g] = 0; w->mv_y[g] = 0;

                ac_pred = (int)ReadOneBit(c);
                (void)ac_pred;

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
                    decode_intra_block_p(c, w, n, coded, bd, bstride);
                }
            }
        }

        // Per-row consumption (P#0): pinpoints where the parse derails.
        if (g_dbg_pframes == 0 && (n_inter + n_intra) > 0)
            DbgPrint("wmv2:   row %d end consumed=%d (inter=%d intra=%d)\n",
                     mb_y, (int)(c->pDecodingPosition - (BYTE *)frame), n_inter, n_intra);
    }

    // Diagnostic: MB composition + bit consumption vs frame size for the first
    // few P-frames. consumed ~ size (within a few bytes of bit-reader lookahead)
    // means the parse stayed in sync; a large mismatch means a desync bug.
    if (g_dbg_pframes < 4) {
        int consumed = (int)(c->pDecodingPosition - (BYTE *)frame);
        int i, nz = 0, tail = (int)size - consumed;
        for (i = consumed; i < (int)size; i++)
            if (frame[i]) nz++;
        DbgPrint("wmv2: P#%d mb skip=%d inter=%d intra=%d | coded=%d abt=%d | "
                 "consumed=%d/%u | tail_nonzero=%d/%d\n",
                 g_dbg_pframes, n_skip, n_inter, n_intra, g_dbg_coded, g_dbg_abt,
                 consumed, size, nz, tail);
    }
    g_dbg_pframes++;

    return 0;
}
