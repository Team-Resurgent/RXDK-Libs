/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Decoder rendering backend: converts the internal YUV 4:2:0 planes of a
 * decoded frame into a locked D3D surface. RenderBitmap picks a converter from
 * the target surface format -- an MMX inner loop for D3DFMT_YUY2 that packs
 * two macroblock rows per pass, or a plain-C BT.601 path for
 * D3DFMT_LIN_A8R8G8B8. The caller crops the coded (macroblock-aligned) frame
 * to the display size.
 */

#include <xtl.h>
#include <xdbg.h>
#include <xmv.h>

#include "decoder.h"

/*
 * Convert our internal YUV format into a standard YUY2 buffer.
 */

static
void RenderToYUY2
(
    DWORD MBWidth, 
    DWORD MBHeight, 
    BYTE *pY, 
    BYTE *pU, 
    BYTE *pV,
    BYTE *pDestination, 
    DWORD DestinationPitch
)
{
    DWORD iWidthY;
    DWORD iWidthUV;
    DWORD PitchAdjust;

    DWORD x;

    // Make sure that our parameters are all 8-byte aligned.
    ASSERT(((DWORD)pY) % 8 == 0); 
    ASSERT(((DWORD)pU) % 8 == 0); 
    ASSERT(((DWORD)pV) % 8 == 0); 
    ASSERT(((DWORD)pDestination) % 8 == 0); 
    ASSERT(DestinationPitch % 8 == 0);

    iWidthY  = MACROBLOCK_SIZE * MBWidth;
    iWidthUV = BLOCK_SIZE * MBWidth;

    PitchAdjust = DestinationPitch - MBWidth * MACROBLOCK_SIZE * 2;

    while(MBHeight--)
    {
        x = MBWidth;

        while (x--)
        {
            __asm
            {
                mov         esi, pY
                mov         edi, pDestination
                mov         ecx, pU
                mov         edx, pV
                mov         eax, BLOCK_SIZE

            L1: movq        mm2, [ecx]          ; 8 U values
                movq        mm3, [edx]          ; 8 V values
                pxor        mm4, mm4
                pxor        mm6, mm6
                pxor        mm0, mm0
                pxor        mm1, mm1
                punpcklbw   mm4, mm2
                punpckhbw   mm6, mm2
                movq        mm5, mm4
                movq        mm7, mm6
                punpckhwd   mm5, mm0            ; spread Us into 4 MMX registers
                punpckhwd   mm7, mm0            ;   ..u...u...u...u...u...u...u...u.
                punpcklwd   mm4, mm0
                punpcklwd   mm6, mm0

                pxor        mm2, mm2
                add         ecx, iWidthUV
                add         edx, iWidthUV

                punpcklbw   mm0, mm3            ; spread Vs and then OR them into Us 
                punpcklwd   mm1, mm0            ;   v.u.v.u.v.u.v.u.v.u.v.u.v.u.v.u.
                punpckhwd   mm2, mm0
                por         mm4, mm1
                por         mm5, mm2

                movq        mm0, [esi]          ; 16 Y values from the current line
                movq        mm1, [esi+8]

                pxor        mm2, mm2
                punpckhbw   mm2, mm3
                pxor        mm3, mm3
                punpcklwd   mm3, mm2
                por         mm6, mm3
                pxor        mm3, mm3
                punpckhwd   mm3, mm2
                por         mm7, mm3

                add         esi, iWidthY        ; spread Ys and output the final results
                pxor        mm2, mm2            ;   vyuyvyuyvyuyvyuyvyuyvyuyvyuyvyuy
                movq        mm3, mm0
                punpcklbw   mm0, mm2
                por         mm0, mm4
                movq        [edi], mm0          ; notice that we output 32 bytes (4 qwords)
                punpckhbw   mm3, mm2            ; without any intervening memory access
                por         mm3, mm5            ; to achieve maximum memory write perf
                movq        [edi+8], mm3
                movq        mm3, mm1
                punpcklbw   mm1, mm2
                por         mm1, mm6
                movq        [edi+16], mm1
                punpckhbw   mm3, mm2
                por         mm3, mm7
                movq        [edi+24], mm3

                movq        mm0, [esi]          ; 16 Y values from the next line
                movq        mm1, [esi+8]
                add         edi, DestinationPitch
                pxor        mm2, mm2
                movq        mm3, mm0
                punpcklbw   mm0, mm2
                por         mm0, mm4
                movq        [edi], mm0
                punpckhbw   mm3, mm2
                por         mm3, mm5
                movq        [edi+8], mm3
                movq        mm3, mm1
                punpcklbw   mm1, mm2
                por         mm1, mm6
                movq        [edi+16], mm1
                punpckhbw   mm3, mm2
                por         mm3, mm7
                movq        [edi+24], mm3

                add         esi, iWidthY
                add         edi, DestinationPitch
                dec         eax
                jnz         L1
            }

            pY += MACROBLOCK_SIZE;
            pU += BLOCK_SIZE;
            pV += BLOCK_SIZE;

            pDestination += MACROBLOCK_SIZE * 2;
        }

        pY += (MACROBLOCK_SIZE - 1) * iWidthY;
        pU += (BLOCK_SIZE - 1) * iWidthUV;
        pV += (BLOCK_SIZE - 1) * iWidthUV;

        pDestination += PitchAdjust + DestinationPitch * (MACROBLOCK_SIZE - 1);
    }

    __asm emms;
}

/*
 * Convert our internal YUV 4:2:0 planes into linear A8R8G8B8 (D3DFMT_LIN_A8R8G8B8),
 * a full render of the coded frame (the caller crops to the display size). BT.601
 * limited-range coefficients. Plain C, for titles that texture the video as ARGB
 * rather than as a YUY2 surface.
 */

static
void RenderToARGB
(
    DWORD MBWidth,
    DWORD MBHeight,
    BYTE *pY,
    BYTE *pU,
    BYTE *pV,
    BYTE *pDestination,
    DWORD DestinationPitch
)
{
    DWORD width   = MBWidth  * MACROBLOCK_SIZE;   // coded luma width  (pY pitch)
    DWORD height  = MBHeight * MACROBLOCK_SIZE;   // coded luma height
    DWORD uvWidth = MBWidth  * BLOCK_SIZE;        // chroma width (pU/pV pitch, 4:2:0)
    DWORD x, y;

    for (y = 0; y < height; y++)
    {
        BYTE       *dst  = pDestination + y * DestinationPitch;
        const BYTE *yrow = pY + y * width;
        const BYTE *urow = pU + (y >> 1) * uvWidth;
        const BYTE *vrow = pV + (y >> 1) * uvWidth;

        for (x = 0; x < width; x++)
        {
            int C = (int)yrow[x]      - 16;
            int D = (int)urow[x >> 1] - 128;
            int E = (int)vrow[x >> 1] - 128;

            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;

            if (R < 0) R = 0; else if (R > 255) R = 255;
            if (G < 0) G = 0; else if (G > 255) G = 255;
            if (B < 0) B = 0; else if (B > 255) B = 255;

            // A8R8G8B8 in memory (little-endian) is B, G, R, A.
            dst[x * 4 + 0] = (BYTE)B;
            dst[x * 4 + 1] = (BYTE)G;
            dst[x * 4 + 2] = (BYTE)R;
            dst[x * 4 + 3] = 0xFF;
        }
    }
}

/*
 * Converts the current YUV buffer into the format we want to display.
 */

void RenderBitmap
(
    XmvVideoCore *pDecoder,
    D3DSurface *pSurface
)
{
    D3DLOCKED_RECT Rect;
    D3DSURFACE_DESC Desc;

    D3DSurface_GetDesc(pSurface, &Desc);
    D3DSurface_LockRect(pSurface, &Rect, 0, D3DLOCK_TILED);

#if DBG

    if (Desc.Width != pDecoder->Width || Desc.Height != pDecoder->Height)
    {
        RIP("The target surface must have exactly the same size as the decoded video.");
    }

#endif DBG

    switch(Desc.Format)
    {
    case D3DFMT_YUY2:

        RenderToYUY2(pDecoder->MBWidth,
                     pDecoder->MBHeight,
                     pDecoder->pYDisplayed,
                     pDecoder->pUDisplayed,
                     pDecoder->pVDisplayed,
                     Rect.pBits,
                     Rect.Pitch);

        break;

    case D3DFMT_LIN_A8R8G8B8:

        RenderToARGB(pDecoder->MBWidth,
                     pDecoder->MBHeight,
                     pDecoder->pYDisplayed,
                     pDecoder->pUDisplayed,
                     pDecoder->pVDisplayed,
                     Rect.pBits,
                     Rect.Pitch);

        break;

    default:
        RIP("Unsupported target surface format, only YUY2 is supported at this time.");
        break;
    }

#if DBG && 0

    // Draw a grid on top of the rendered frame so that we can figure out what
    // macroblocks contain the drawing errors.
    //
    {
        WORD *pPixel;
        DWORD x, y;

        for (y = 0; y < Desc.Height; y++)
        {
            pPixel = (WORD *)((BYTE *)Rect.pBits + y * Rect.Pitch);

            // Double the middle line.
            if (y % 16 == 0 || y == Desc.Height / 2 + 1)
            {
                for (x = 0; x < Desc.Width; x++)
                {
                    *pPixel = (*pPixel & 0xFF00) | 0x40;

                    pPixel++;
                }
            }
            else
            {
                for (x = 0; x < Desc.Width; x++)
                {
                    // Double the middle line.
                    if (x % 16 == 0 || x == Desc.Width / 2 + 1)
                    {
                        *pPixel = (*pPixel & 0xFF00) | 0x40;
                    }

                    pPixel++;
                }
            }
        }
    }

#endif DBG

    D3DSurface_UnlockRect(pSurface);
}

