//------------------------------------------------------------------------------
// wmv2dec.h -- WMV2 P-frame decode layer over the leak XMV video kernel.
//
// Holds the WMV2 sequence options (from the 4-byte extradata) and the per-frame
// picture-header state, plus the ported MV / MB-type VLCs. Increment 1 covers
// the sequence + picture-header parse (the MB loop + motion comp are increment 2).
// All bit reads go through XmvVideoCore's bit walker (ReadBits/ReadOneBit/...).
//------------------------------------------------------------------------------
#ifndef RXDK_WMV2DEC_H
#define RXDK_WMV2DEC_H

#include <stdint.h>
#include "wmv2_vlc.h"

struct XmvVideoCore;

// Picture types (match FFmpeg AV_PICTURE_TYPE_I/P: pict_type = first bit + 1).
#define WMV2_PICT_I  1
#define WMV2_PICT_P  2

typedef struct Wmv2 {
    struct XmvVideoCore *core;

    // Sequence options (decode_ext_header).
    int mspel_bit, loop_filter, abt_flag, j_type_bit, top_left_mv_flag, per_mb_rl_bit;
    int slice_code, bit_rate;

    // Per-frame picture header.
    int pict_type;          // WMV2_PICT_I / _P
    int qscale;
    int no_rounding;        // stateful: I sets 1, P toggles
    int j_type;
    int per_mb_rl_table, rl_table_index, rl_chroma_table_index;
    int dc_table_index, mv_table_index;
    int mspel, cbp_table_index;
    int hshift;             // mspel half-pel-shift bit (per MV)
    int per_mb_abt, abt_type, per_block_abt;
    int esc3_run_length, esc3_level_length;

    // Per-MB skip flags (1 = skipped), [MBWidth*MBHeight].
    uint8_t *mb_skip;

    // Per-MB motion vectors with a 1-MB border (left/top/right) for prediction.
    // stride = MBWidth+2; index(mb_x,mb_y) = (mb_y+1)*stride + (mb_x+1).
    int16_t *mv_x;          // [(MBWidth+2)*(MBHeight+1)]
    int16_t *mv_y;
    int      mv_stride;

    // Intra DC-prediction grids (dequantized DC of each 8x8 block, b8 resolution
    // for luma, MB resolution for chroma; 1-block top/left border = 1024). Reset
    // to 1024 each frame; only intra blocks write. Used by the in-P intra path.
    int16_t *dc_y, *dc_u, *dc_v;
    int      dc_y_stride, dc_c_stride;

    // Intra AC-prediction grids: 16 coeffs per block (1..7 = left column, 8+1..8+7
    // = top row), same grid layout/stride as dc_*. Reset to 0 each frame.
    int16_t *ac_y, *ac_u, *ac_v;

    // Ported VLC tables.
    Wmv2Vlc mv_vlc[2];      // by mv_table_index
    Wmv2Vlc cbp_vlc[4];     // by cbp_table_index (P MB-type + CBP)

    int initialized;
} Wmv2;

// Build the WMV2 layer: decode the 4-byte sequence extradata and build the VLCs.
// Returns 0 on success, <0 on failure.
int  Wmv2Init(Wmv2 *w, struct XmvVideoCore *core, const uint8_t extradata[4]);
void Wmv2Free(Wmv2 *w);

// Parse the primary picture header (pict_type + qscale [+ I 7-bit field]).
// Returns WMV2_PICT_I / _P, or <0 on error.
int  Wmv2DecodePictureHeader(Wmv2 *w);

// Parse the secondary picture header (RL/DC/MV table indices, mspel, ABT, and
// for P the macroblock skip bitmap). Call after Wmv2DecodePictureHeader.
// Returns 0 on success, <0 on error.
int  Wmv2DecodeSecondaryHeader(Wmv2 *w);

// Decode a whole P-frame: assumes the bit walker is positioned at the start of
// the frame and the primary+secondary headers have already been parsed. Motion-
// compensates from the core's displayed planes (reference) into its building
// planes and adds the inter/intra residual. Returns 0 on success, <0 on error.
int  Wmv2DecodePFrame(Wmv2 *w, const unsigned char *frame, unsigned size);

// Reset the motion-vector grid (called at I-frames: no MVs to predict from).
void Wmv2ResetMotion(Wmv2 *w);

#endif // RXDK_WMV2DEC_H
