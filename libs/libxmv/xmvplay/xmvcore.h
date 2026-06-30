//------------------------------------------------------------------------------
// xmvcore.h -- thin wrapper around the leak XMV video decode kernel.
//
// The leak software codec (decoder/frontend.c + backend.c + bits.c + huffman.c
// + tables.c) decodes one WMV2 *I-frame* into YUV planes and converts them to a
// YUY2 D3D surface. Its P-frame path is unimplemented in the leaked source, so
// this wrapper only drives keyframes (Phase 2 experiment: prove the container +
// codec wiring by rendering keyframe images; full I+P is the FFmpeg port).
//
// XmvVideoCore is the leak decode context (struct defined in decoder/decoder.h).
// The wrapper owns its frame-buffer allocation (mirrors decoder.c minus file IO)
// and feeds the demuxer's already-dword-reversed frame body to the bit walker.
//------------------------------------------------------------------------------

#ifndef RXDK_XMVCORE_H
#define RXDK_XMVCORE_H

// The leak decode context; full layout lives in decoder/decoder.h.
typedef struct XmvVideoCore XmvVideoCore;

// Create a decode core for a width x height video (both must be /16). xintra8 is
// the sequence-level XINTRA8 I-picture coding flag (consumed by DecodeIFrame).
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
