/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Linker section setup for xgraphics.lib. Places code, data, bss and const
 * into the XGRPH segments, merges them into a single read/write code section,
 * and stamps the library build-number marker (XGraphicsBuildNumber, or the
 * debug variant) into the .XBLD$V version section.
 */

#ifndef __XGRPHSEG_H__
#define __XGRPHSEG_H__

#include "xboxverp.h"

#pragma code_seg("XGRPH")
#pragma data_seg("XGRPH_RW")
#pragma bss_seg("XGRPH_RW")
#pragma const_seg("XGRPH_RD")

// Tell the linker to merge constant data and data sections into code section.
#pragma comment(linker, "/merge:XGRPH_RD=XGRPH")
#pragma comment(linker, "/merge:XGRPH_RW=XGRPH")

// Tell the linker that the code section contains read/write data.
#pragma comment(linker, "/section:XGRPH,ERW")

#ifdef __cplusplus
extern "C"
{
#endif

#pragma data_seg(".XBLD$V")

#if DBG
#pragma comment(linker, "/include:_XGraphicsBuildNumberD")
__declspec(selectany) unsigned short XGraphicsBuildNumberD[8] =
        { 'X' | ('G' << 8), 'R' | ('A' << 8), 'P' | ('H' << 8), 'C' | ('D' << 8),
          VER_PRODUCTVERSION };
#else
#pragma comment(linker, "/include:_XGraphicsBuildNumber")
__declspec(selectany) unsigned short XGraphicsBuildNumber[8] =
        { 'X' | ('G' << 8), 'R' | ('A' << 8), 'P' | ('H' << 8), 'C',
          VER_PRODUCTVERSION };
#endif

#pragma data_seg()
#ifdef __cplusplus
}
#endif

#endif //__XGRPHSEG_H__
