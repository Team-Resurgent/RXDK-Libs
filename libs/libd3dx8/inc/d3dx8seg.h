/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Section-layout control for the D3DX8 library. Directs the compiler to place
 * D3DX8's code, data and constants into a merged D3DX section marked
 * executable/read/write, and emits the D3DX8 build-number stamp into the .XBLD
 * version section (debug and retail variants) so the library's version is
 * recorded in the linked image.
 */

#ifndef __D3DX8SEG_H__
#define __D3DX8SEG_H__

#include "xboxverp.h"

#pragma code_seg("D3DX")
#pragma data_seg("D3DX_RW")
#pragma const_seg("D3DX_RD")

// Tell the linker to merge constant data and data sections into code section.
#pragma comment(linker, "/merge:D3DX_RD=D3DX")
#pragma comment(linker, "/merge:D3DX_RW=D3DX")

// Tell the linker that the code section contains read/write data.
#pragma comment(linker, "/section:D3DX,ERW")

#ifdef __cplusplus
extern "C"
{
#endif

#pragma data_seg(".XBLD$V")

#if DBG
#pragma comment(linker, "/include:_D3DX8BuildNumberD")
__declspec(selectany) unsigned short D3DX8BuildNumberD[8] =
        { 'D' | ('3' << 8), 'D' | ('X' << 8), '8' | ('D' << 8), 0,
          VER_PRODUCTVERSION };
#else
#pragma comment(linker, "/include:_D3DX8BuildNumber")
__declspec(selectany) unsigned short D3DX8BuildNumber[8] =
        { 'D' | ('3' << 8), 'D' | ('X' << 8), '8', 0,
          VER_PRODUCTVERSION };
#endif

#pragma data_seg()
#ifdef __cplusplus
}
#endif

#endif //__D3DX8SEG_H__
