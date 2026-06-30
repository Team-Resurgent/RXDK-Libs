//------------------------------------------------------------------------------
// xmvcore.c -- wrapper that drives the leak XMV video decode kernel (see
// xmvcore.h). Allocates the decode context's frame buffers + predictor arrays
// (mirrors decoder.c's XMVCreateDecoder, minus the overlapped file IO) and feeds
// the demuxer's dword-reversed WMV2 frame bodies to the bit walker.
//------------------------------------------------------------------------------

#include <xtl.h>
#include <d3d8.h>

#include "decoder.h"     // full XmvVideoCore layout + DecodeOneFrame/RenderBitmap

// MACROBLOCK_SIZE (16) / BLOCK_SIZE (8) come from decoder.h.

static BYTE *AlignUp16(BYTE *p)
{
    return (BYTE *)(((DWORD)p + 15) & ~(DWORD)15);
}

XmvVideoCore *XmvCoreCreate(unsigned width, unsigned height, int xintra8_enabled)
{
    XmvVideoCore *c;
    DWORD mbw, mbh, uvw, uvh;
    DWORD aux_bytes, plane_bytes;
    BYTE *aux, *walk, *planes_raw, *planes;

    if (width == 0 || height == 0 || (width % MACROBLOCK_SIZE) || (height % MACROBLOCK_SIZE))
        return NULL;

    c = (XmvVideoCore *)malloc(sizeof(*c));
    if (!c)
        return NULL;
    memset(c, 0, sizeof(*c));

    c->Width    = width;
    c->Height   = height;
    c->UVWidth  = width / 2;
    c->UVHeight = height / 2;
    c->MBWidth  = width / MACROBLOCK_SIZE;
    c->MBHeight = height / MACROBLOCK_SIZE;
    c->XIntra8IPictureCodingEnabled = xintra8_enabled ? TRUE : FALSE;

    mbw = c->MBWidth;
    mbh = c->MBHeight;
    uvw = c->UVWidth;
    uvh = c->UVHeight;

    // Predictor scratch: CBPCY row + Y/U/V AC-predictor rows (see decoder.c).
    aux_bytes = sizeof(XMVMacroblockCBPCY) * (mbw + 1)
              + sizeof(short) * mbw * MACROBLOCK_SIZE   // pYAC
              + sizeof(short) * mbw * BLOCK_SIZE        // pUAC
              + sizeof(short) * mbw * BLOCK_SIZE;       // pVAC
    aux = (BYTE *)malloc(aux_bytes);
    if (!aux) {
        free(c);
        return NULL;
    }
    memset(aux, 0, aux_bytes);

    walk = aux;
    c->pCBPCY = (XMVMacroblockCBPCY *)walk;
    walk += sizeof(XMVMacroblockCBPCY) * (mbw + 1);
    c->pYAC = (short *)walk;
    walk += sizeof(short) * mbw * MACROBLOCK_SIZE;
    c->pUAC = (short *)walk;
    walk += sizeof(short) * mbw * BLOCK_SIZE;
    c->pVAC = (short *)walk;

    // Frame planes: displayed + building, each Y + U + V. Over-allocate for a
    // 16-byte aligned base (backend.c's MMX YUY2 converter wants 8-byte aligned
    // plane pointers; all plane offsets here are multiples of 8).
    plane_bytes = 2 * (width * height + 2 * uvw * uvh);
    planes_raw = (BYTE *)malloc(plane_bytes + 16);
    if (!planes_raw) {
        free(aux);
        free(c);
        return NULL;
    }
    planes = AlignUp16(planes_raw);

    walk = planes;
    c->pYDisplayed = walk; walk += width * height;
    c->pUDisplayed = walk; walk += uvw * uvh;
    c->pVDisplayed = walk; walk += uvw * uvh;
    c->pYBuilding  = walk; walk += width * height;
    c->pUBuilding  = walk; walk += uvw * uvh;
    c->pVBuilding  = walk;

    // Neutral initial frame (black) so a render before the first keyframe is not
    // garbage: Y=0x10, U/V=0x80.
    memset(c->pYDisplayed, 0x10, width * height);
    memset(c->pUDisplayed, 0x80, uvw * uvh);
    memset(c->pVDisplayed, 0x80, uvw * uvh);
    memset(c->pYBuilding,  0x10, width * height);
    memset(c->pUBuilding,  0x80, uvw * uvh);
    memset(c->pVBuilding,  0x80, uvw * uvh);

    // Stash the raw allocation bases (these struct fields are unused on our
    // file-less path) so XmvCoreDestroy can free them.
    c->pLoadingBuffer  = aux;
    c->pDecodingBuffer = planes_raw;

    return c;
}

void XmvCoreDestroy(XmvVideoCore *core)
{
    if (!core)
        return;
    if (core->pDecodingBuffer) free(core->pDecodingBuffer);  // planes_raw
    if (core->pLoadingBuffer)  free(core->pLoadingBuffer);   // aux
    free(core);
}

void XmvCoreDecodeKeyframe(XmvVideoCore *core, const unsigned char *data, unsigned size)
{
    BYTE *swap;

    (void)size;
    if (!core || !data)
        return;

    // Point the bit walker at the (already dword-reversed) frame body.
    core->pDecodingPosition = (BYTE *)data;
    core->BitCache          = 0;
    core->BitsRemaining     = 0;

    // Decode into the "building" planes (I-frame path; the leading I/P bit of a
    // keyframe is 0, so DecodeOneFrame takes DecodeIFrame).
    DecodeOneFrame(core);

    // Promote building -> displayed (what RenderBitmap shows).
    swap = core->pYDisplayed; core->pYDisplayed = core->pYBuilding; core->pYBuilding = swap;
    swap = core->pUDisplayed; core->pUDisplayed = core->pUBuilding; core->pUBuilding = swap;
    swap = core->pVDisplayed; core->pVDisplayed = core->pVBuilding; core->pVBuilding = swap;
}

void XmvCoreRender(XmvVideoCore *core, void *pSurface)
{
    if (!core || !pSurface)
        return;
    RenderBitmap(core, (D3DSurface *)pSurface);
}
