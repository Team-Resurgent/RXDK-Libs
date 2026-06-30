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

// decode_ext_header (wmv2dec.c): 32-bit sequence header.
static void wmv2_decode_ext_header(Wmv2 *w, const uint8_t ext[4])
{
    ExtBits b;
    b.p = ext; b.pos = 0;

    (void)ext_get(&b, 5);                       // fps
    w->bit_rate         = (int)ext_get(&b, 11) * 1024;
    w->mspel_bit        = (int)ext_get(&b, 1);
    w->loop_filter      = (int)ext_get(&b, 1);
    w->abt_flag         = (int)ext_get(&b, 1);
    w->j_type_bit       = (int)ext_get(&b, 1);
    w->top_left_mv_flag = (int)ext_get(&b, 1);
    w->per_mb_rl_bit    = (int)ext_get(&b, 1);
    w->slice_code       = (int)ext_get(&b, 3);
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
static int wmv2_parse_mb_skip(Wmv2 *w)
{
    XmvVideoCore *c = w->core;
    int mbw = (int)c->MBWidth, mbh = (int)c->MBHeight;
    uint8_t *skip = w->mb_skip;
    int x, y, skip_type;

    skip_type = (int)ReadBits(c, 2);
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
            if (ReadOneBit(c)) {
                for (x = 0; x < mbw; x++) skip[y * mbw + x] = 1;
            } else {
                for (x = 0; x < mbw; x++) skip[y * mbw + x] = (uint8_t)ReadOneBit(c);
            }
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
