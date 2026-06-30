//------------------------------------------------------------------------------
// wmv2dec.c -- WMV2 sequence + picture header parse (see wmv2dec.h). Ported from
// FFmpeg libavcodec/wmv2dec.c (decode_ext_header / wmv2_decode_picture_header /
// ff_wmv2_decode_secondary_picture_header) and msmpeg4dec.c, retargeted onto the
// leak XMV bit walker. Increment 1 of the WMV2 P-frame port.
//------------------------------------------------------------------------------

#include <xtl.h>
#include <xdbg.h>

#include "decoder.h"        // XmvVideoCore + ReadBits/ReadOneBit/ReadTriStateBits/PeekBits/SkipBits
#include "wmv2dec.h"
#include "wmv2_tables.h"

#define SKIP_TYPE_NONE 0
#define SKIP_TYPE_MPEG 1
#define SKIP_TYPE_ROW  2
#define SKIP_TYPE_COL  3

// FFmpeg decode012 == leak ReadTriStateBits (1 bit; if set, 1 + next bit).
#define decode012(c) ((int)ReadTriStateBits(c))

// ---- tiny MSB-first reader over the 4-byte sequence extradata ---------------
typedef struct { const uint8_t *p; int pos; } ExtBits;
static unsigned ext_get(ExtBits *b, int n)
{
    unsigned v = 0;
    while (n--) {
        int bit = (b->p[b->pos >> 3] >> (7 - (b->pos & 7))) & 1;
        v = (v << 1) | (unsigned)bit;
        b->pos++;
    }
    return v;
}

// Decode the XMV sequence extradata. This is the RAW XMV format: a little-endian
// uint32 bitfield (mspel=bit0, loop=bit1, abt=bit2, j_type=bit3, top_left_mv=bit4,
// per_mb_rl=bit5, slice_count=bits6..8) -- NOT the "standard WMV2 extradata"
// layout. FFmpeg's demuxer (libavformat/xmv.c xmv_read_extradata) repacks it into
// the WMV2 layout before its decoder reads it; we decode the raw bitfield directly.
static void wmv2_decode_ext_header(Wmv2 *w, const uint8_t ext[4])
{
    uint32_t data = (uint32_t)ext[0] | ((uint32_t)ext[1] << 8) |
                    ((uint32_t)ext[2] << 16) | ((uint32_t)ext[3] << 24);

    w->mspel_bit        = (int)((data >> 0) & 1);
    w->loop_filter      = (int)((data >> 1) & 1);
    w->abt_flag         = (int)((data >> 2) & 1);
    w->j_type_bit       = (int)((data >> 3) & 1);
    w->top_left_mv_flag = (int)((data >> 4) & 1);
    w->per_mb_rl_bit    = (int)((data >> 5) & 1);
    w->slice_code       = (int)((data >> 6) & 7);
    w->bit_rate         = 0;   // not encoded in the raw XMV extradata
}

int Wmv2Init(Wmv2 *w, XmvVideoCore *core, const uint8_t extradata[4])
{
    int rc;

    memset(w, 0, sizeof(*w));
    w->core = core;

    wmv2_decode_ext_header(w, extradata);

    w->mb_skip = (uint8_t *)malloc((size_t)core->MBWidth * core->MBHeight);
    if (!w->mb_skip)
        return -1;

    w->mv_stride = (int)core->MBWidth + 2;
    {
        size_t n = (size_t)w->mv_stride * (core->MBHeight + 1);
        w->mv_x = (int16_t *)malloc(n * sizeof(int16_t));
        w->mv_y = (int16_t *)malloc(n * sizeof(int16_t));
        if (!w->mv_x || !w->mv_y)
            return -1;
    }

    // Intra DC-prediction grids: luma at b8 resolution (2 blocks/MB) + 1-block
    // border, chroma at MB resolution + 1-block border.
    w->dc_y_stride = (int)core->MBWidth * 2 + 2;
    w->dc_c_stride = (int)core->MBWidth + 2;
    {
        size_t ny = (size_t)w->dc_y_stride * (core->MBHeight * 2 + 2);
        size_t nc = (size_t)w->dc_c_stride * (core->MBHeight + 2);
        w->dc_y = (int16_t *)malloc(ny * sizeof(int16_t));
        w->dc_u = (int16_t *)malloc(nc * sizeof(int16_t));
        w->dc_v = (int16_t *)malloc(nc * sizeof(int16_t));
        w->ac_y = (int16_t *)malloc(ny * 16 * sizeof(int16_t));
        w->ac_u = (int16_t *)malloc(nc * 16 * sizeof(int16_t));
        w->ac_v = (int16_t *)malloc(nc * 16 * sizeof(int16_t));
        if (!w->dc_y || !w->dc_u || !w->dc_v || !w->ac_y || !w->ac_u || !w->ac_v)
            return -1;
    }

    rc  = Wmv2VlcBuildFromLengths(&w->mv_vlc[0], ff_msmp4_mv_table0_lens, ff_msmp4_mv_table0,
                                  MSMPEG4_MV_TABLES_NB_ELEMS);
    rc |= Wmv2VlcBuildFromLengths(&w->mv_vlc[1], ff_msmp4_mv_table1_lens, ff_msmp4_mv_table1,
                                  MSMPEG4_MV_TABLES_NB_ELEMS);
    rc |= Wmv2VlcBuildExplicit(&w->cbp_vlc[0], table_mb_non_intra2, 128);
    rc |= Wmv2VlcBuildExplicit(&w->cbp_vlc[1], table_mb_non_intra3, 128);
    rc |= Wmv2VlcBuildExplicit(&w->cbp_vlc[2], table_mb_non_intra4, 128);
    rc |= Wmv2VlcBuildExplicit(&w->cbp_vlc[3], ff_table_mb_non_intra, 128);
    if (rc != 0)
        return -1;

    w->no_rounding  = 0;
    w->initialized  = 1;

    DbgPrint("wmv2: ext mspel=%d loop=%d abt=%d jtype=%d tlmv=%d permbrl=%d slices=%d br=%dk\n",
             w->mspel_bit, w->loop_filter, w->abt_flag, w->j_type_bit,
             w->top_left_mv_flag, w->per_mb_rl_bit, w->slice_code, w->bit_rate / 1024);
    return 0;
}

void Wmv2Free(Wmv2 *w)
{
    int i;
    if (!w->initialized)
        return;
    if (w->mb_skip) free(w->mb_skip);
    if (w->mv_x)    free(w->mv_x);
    if (w->mv_y)    free(w->mv_y);
    if (w->dc_y)    free(w->dc_y);
    if (w->dc_u)    free(w->dc_u);
    if (w->dc_v)    free(w->dc_v);
    if (w->ac_y)    free(w->ac_y);
    if (w->ac_u)    free(w->ac_u);
    if (w->ac_v)    free(w->ac_v);
    for (i = 0; i < 2; i++) Wmv2VlcFree(&w->mv_vlc[i]);
    for (i = 0; i < 4; i++) Wmv2VlcFree(&w->cbp_vlc[i]);
    w->initialized = 0;
}

// wmv2_decode_picture_header (wmv2dec.c). The bit walker must be positioned at
// the start of the frame body.
int Wmv2DecodePictureHeader(Wmv2 *w)
{
    XmvVideoCore *c = w->core;
    int code;

    w->pict_type = (int)ReadOneBit(c) + 1;       // 1 = I, 2 = P
    if (w->pict_type == WMV2_PICT_I) {
        code = (int)ReadBits(c, 7);              // I7 marker (logged, unused)
        (void)code;
    }
    w->qscale = (int)ReadBits(c, 5);
    if (w->qscale <= 0)
        return -1;

    // NOTE: FFmpeg's optional "all-skip => FRAME_SKIPPED" peek is omitted here
    // (it does not consume bits in the normal case); handled in increment 2.
    return w->pict_type;
}

// parse_mb_skip (wmv2dec.c): fills w->mb_skip[mb_y*mb_width + mb_x].
static int g_dbg_skip_frames;

static int wmv2_parse_mb_skip(Wmv2 *w)
{
    XmvVideoCore *c = w->core;
    int mbw = (int)c->MBWidth, mbh = (int)c->MBHeight;
    uint8_t *skip = w->mb_skip;
    int x, y, skip_type;

    skip_type = (int)ReadBits(c, 2);
    if (g_dbg_skip_frames == 0) {
        DbgPrint("wmv2:   P-frame skip_type=%d q=%d\n", skip_type, w->qscale);
        g_dbg_skip_frames++;
    }
    switch (skip_type) {
    case SKIP_TYPE_NONE:
        for (y = 0; y < mbh; y++)
            for (x = 0; x < mbw; x++)
                skip[y * mbw + x] = 0;
        break;
    case SKIP_TYPE_MPEG:
        for (y = 0; y < mbh; y++)
            for (x = 0; x < mbw; x++)
                skip[y * mbw + x] = (uint8_t)ReadOneBit(c);
        break;
    case SKIP_TYPE_ROW:
        for (y = 0; y < mbh; y++) {
            int rb = (int)ReadOneBit(c), nc = 0;
            if (rb) {
                for (x = 0; x < mbw; x++) skip[y * mbw + x] = 1;
            } else {
                for (x = 0; x < mbw; x++) { int s = (int)ReadOneBit(c); skip[y * mbw + x] = (uint8_t)s; nc += !s; }
            }
            (void)nc;
        }
        break;
    case SKIP_TYPE_COL:
        for (x = 0; x < mbw; x++) {
            if (ReadOneBit(c)) {
                for (y = 0; y < mbh; y++) skip[y * mbw + x] = 1;
            } else {
                for (y = 0; y < mbh; y++) skip[y * mbw + x] = (uint8_t)ReadOneBit(c);
            }
        }
        break;
    }
    return 0;
}

// ff_wmv2_decode_secondary_picture_header (wmv2dec.c).
int Wmv2DecodeSecondaryHeader(Wmv2 *w)
{
    XmvVideoCore *c = w->core;

    if (w->pict_type == WMV2_PICT_I) {
        if (w->j_type_bit)
            w->j_type = (int)ReadOneBit(c);
        else
            w->j_type = 0;

        if (!w->j_type) {
            if (w->per_mb_rl_bit)
                w->per_mb_rl_table = (int)ReadOneBit(c);
            else
                w->per_mb_rl_table = 0;

            if (!w->per_mb_rl_table) {
                w->rl_chroma_table_index = decode012(c);
                w->rl_table_index        = decode012(c);
            }
            w->dc_table_index = (int)ReadOneBit(c);
        }
        w->no_rounding = 1;
    } else {
        int cbp_index;

        w->j_type = 0;
        if (wmv2_parse_mb_skip(w) < 0)
            return -1;

        cbp_index = decode012(c);
        w->cbp_table_index = wmv2_get_cbp_table_index(w->qscale, cbp_index);

        if (w->mspel_bit)
            w->mspel = (int)ReadOneBit(c);
        else
            w->mspel = 0;

        if (w->abt_flag) {
            w->per_mb_abt = (int)ReadOneBit(c) ^ 1;
            if (!w->per_mb_abt)
                w->abt_type = decode012(c);
        }

        if (w->per_mb_rl_bit)
            w->per_mb_rl_table = (int)ReadOneBit(c);
        else
            w->per_mb_rl_table = 0;

        if (!w->per_mb_rl_table) {
            w->rl_table_index        = decode012(c);
            w->rl_chroma_table_index = w->rl_table_index;
        }

        w->dc_table_index = (int)ReadOneBit(c);
        w->mv_table_index = (int)ReadOneBit(c);

        w->no_rounding ^= 1;
    }

    w->esc3_level_length = 0;
    w->esc3_run_length   = 0;
    return 0;
}
