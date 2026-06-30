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
    int per_mb_abt, abt_type, per_block_abt;
    int esc3_run_length, esc3_level_length;

    // Per-MB skip flags (1 = skipped), [MBWidth*MBHeight].
    uint8_t *mb_skip;

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

#endif // RXDK_WMV2DEC_H
