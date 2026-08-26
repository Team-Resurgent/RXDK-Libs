/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * dmusver.h -- embeds the DirectMusic build-number stamp into the .XBLD version
 * segment. Selects the debug, profile or retail variant of the build string
 * from the shared xboxverp.h version macros.
 */

#include <xboxverp.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#pragma data_seg(".XBLD$V")

#if DBG
#pragma comment(linker, "/include:_DMusicBuildNumberD")
__declspec(selectany) unsigned short DMusicBuildNumberD[8] = { 'D' | ('M' << 8), 'U' | ('S' << 8), 'I' | ('C' << 8), 'D',
                                        VER_PRODUCTVERSION };
#elif PROFILE
#pragma comment(linker, "/include:_DMusicBuildNumberI")
__declspec(selectany) unsigned short DMusicBuildNumberI[8] = { 'D' | ('M' << 8), 'U' | ('S' << 8), 'I' | ('C' << 8), 'I',
                                        VER_PRODUCTVERSION };
#elif LTCG
#pragma comment(linker, "/include:_DMusicBuildNumberL")
__declspec(selectany) unsigned short DMusicBuildNumberL[8] = { 'D' | ('M' << 8), 'U' | ('S' << 8), 'I' | ('C' << 8), 'L' | ('T' << 8),
                                        VER_PRODUCTVERSION };
#else
#pragma comment(linker, "/include:_DMusicBuildNumber")
__declspec(selectany) unsigned short DMusicBuildNumber[8] = { 'D' | ('M' << 8), 'U' | ('S' << 8), 'I' | ('C' << 8), 0,
                                        VER_PRODUCTVERSION };
#endif

#pragma data_seg()
#ifdef __cplusplus
}
#endif // __cplusplus

