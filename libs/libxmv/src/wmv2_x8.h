//------------------------------------------------------------------------------
// wmv2_x8.h -- IntraX8 (WMV2 "J-frame" / XINTRA8) keyframe decoder, ported from
// FFmpeg libavcodec/intrax8.c + intrax8dsp.c (LGPL) onto the leak XMV kernel's
// bit walker and frame planes.
//
// Modern XMV encoders set j_type_bit in the sequence extradata, giving every
// I-frame a 1-bit j_type flag; when that flag is 1 the keyframe is X8-coded,
// which the leak software decoder never implemented (its XINTRA8 branch is an
// int3 stub). This is the open-source port of that missing path: an all-intra
// frame coded as 8x8 blocks with 12 directional spatial predictors, its own
// DC/AC/orientation VLCs, an optional per-position quant matrix and an
// optional in-loop deblocking filter, reconstructed with the WMV2 integer
// IDCT.
//------------------------------------------------------------------------------
#ifndef RXDK_WMV2_X8_H
#define RXDK_WMV2_X8_H

#include <stdint.h>

struct XmvVideoCore;

typedef struct Wmv2X8 {
    struct XmvVideoCore *core;

    // Lazily-selected VLC tables for the current frame (indices into the
    // static built table sets; NULL until the bitstream selects them).
    const void *ac_vlc[4];      // [ac_mode]
    const void *orient_vlc;
    const void *dc_vlc[3];      // [dc_mode]

    int use_quant_matrix;

    // 2 * (mb_width * 2) bytes: per-8x8-block orient/est_run prediction state
    // for the current and previous block row.
    uint8_t *prediction_table;

    int16_t block[64];

    // Per frame.
    int quant;
    int dquant;
    int qsum;
    int loopfilter;
    int quant_dc_chroma;
    int divide_quant_dc_luma;
    int divide_quant_dc_chroma;
    uint8_t *dest[3];
    int linesize[2];            // [0] = luma pitch, [1] = chroma pitch
    uint8_t scratchpad[42];     // edge-pixel buffer for the spatial predictors

    // Per block.
    int edges;
    int flat_dc;
    int predicted_dc;
    int raw_orient;
    int chroma_orient;
    int orient;
    int est_run;

    int mb_x, mb_y;
    int mb_width, mb_height;

    // End of the current frame's bitstream (for the per-row bits-left check).
    const uint8_t *bit_end;

    int initialized;
} Wmv2X8;

// Build the X8 context over the core's geometry (allocates the prediction
// row and builds the static VLC sets on first use). Returns 0 on success.
int  Wmv2X8Init(Wmv2X8 *w, struct XmvVideoCore *core);
void Wmv2X8Free(Wmv2X8 *w);

// Decode one X8 frame into the core's BUILDING planes (caller swaps). The bit
// walker must be positioned just past the picture header (after the j_type
// bit); `qscale` from the picture header; `loopfilter` is the sequence
// loop-filter flag; `bit_end` bounds the frame's bitstream. Returns 0 on
// success (partial decodes still return 0, mirroring FFmpeg).
int Wmv2X8DecodeFrame(Wmv2X8 *w, int qscale, int loopfilter,
                      const uint8_t *bit_end);

#endif // RXDK_WMV2_X8_H
