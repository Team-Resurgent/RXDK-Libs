/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

//------------------------------------------------------------------------------
// xmvcore.h -- thin wrapper around the XMV video decode kernel.
//
// The baseline XMV kernel (decoder/frontend.c + backend.c + bits.c + huffman.c
// + tables.c) decodes one WMV2 *I-frame* into YUV planes and converts them to a
// YUY2 D3D surface; it does not implement WMV2 P-frames, so this wrapper drives
// the keyframe path (the P-frame path lives in the wmv2*.c modules and is
// driven through XmvCoreSetupBits / XmvCoreSwap).
//
// XmvVideoCore is the decode context (struct defined in decoder/decoder.h).
// The wrapper owns its frame-buffer allocation (mirrors decoder.c minus file IO)
// and feeds the demuxer's already-dword-reversed frame body to the bit walker.
//------------------------------------------------------------------------------

#ifndef RXDK_XMVCORE_H
#define RXDK_XMVCORE_H

// The decode context; full layout lives in decoder/decoder.h.
typedef struct XmvVideoCore XmvVideoCore;

// Create a decode core for a width x height video. Dimensions are aligned up
// to 16 (macroblock) multiples internally; the rendered output covers the
// aligned size, so the caller crops to the display size. xintra8 is the
// sequence-level XINTRA8 I-picture coding flag (consumed by DecodeIFrame).
// Returns NULL on allocation failure.
XmvVideoCore *XmvCoreCreate(unsigned width, unsigned height, int xintra8_enabled);

void XmvCoreDestroy(XmvVideoCore *core);

// Decode one keyframe: `data` is the dword-reversed WMV2 frame body (from the
// demuxer), `size` its length. Updates the displayed YUV planes. Caller must
// only pass I-frames (P-frames are not implemented).
void XmvCoreDecodeKeyframe(XmvVideoCore *core, const unsigned char *data, unsigned size);

// Convert the currently displayed YUV planes to YUY2 into a D3DSurface (passed
// as void* to keep this header free of the d3d8 umbrella).
void XmvCoreRender(XmvVideoCore *core, void *pSurface);

// Point the bit walker at a (dword-reversed) frame body, resetting the cache.
// Used to drive the WMV2 header/MB parse (wmv2dec.c) over the core's bit reader.
void XmvCoreSetupBits(XmvVideoCore *core, const unsigned char *data);

// Promote the just-built planes to the displayed (reference) planes. Used by the
// P-frame path, which decodes into the building planes from the displayed ones.
void XmvCoreSwap(XmvVideoCore *core);

#endif // RXDK_XMVCORE_H
