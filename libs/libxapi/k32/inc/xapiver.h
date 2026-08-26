/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Emits the XAPI build-number version stamp into the .XBLD linker section. The
 * stamped symbol name is chosen by build flavor - the process (XAPILIBP) versus
 * import-library build, and the debug (DBG) versus retail build - and the kernel
 * build-number symbol is force-referenced so it is linked in.
 */

#include <xboxverp.h>

#pragma comment(linker, "/include:_XboxKrnlBuildNumber")

#pragma data_seg(".XBLD$A")
#ifdef XAPILIBP

#if DBG
#pragma comment(linker, "/include:_XapiBuildNumberPD")
__declspec(selectany) USHORT XapiBuildNumberPD[8] = { 'AX', 'IP', 'BL', 'DP',
    VER_PRODUCTVERSION
#else  // DBG
#pragma comment(linker, "/include:_XapiBuildNumberP")
__declspec(selectany) USHORT XapiBuildNumberP[8] = { 'AX', 'IP', 'BL', 'P',
    VER_PRODUCTVERSION
#endif // DBG

#else  // XAPILIBP

#if DBG
#pragma comment(linker, "/include:_XapiBuildNumberD")
__declspec(selectany) USHORT XapiBuildNumberD[8] = { 'AX', 'IP', 'IL', 'DB',
    VER_PRODUCTVERSION
#else  // DBG
#pragma comment(linker, "/include:_XapiBuildNumber")
__declspec(selectany) USHORT XapiBuildNumber[8] = { 'AX', 'IP', 'IL', 'B',
    VER_PRODUCTVERSION
#endif // DBG

#endif // XAPILIBP
};
#pragma comment(linker, "/SECTION:.XBLD,D")
#pragma data_seg()
