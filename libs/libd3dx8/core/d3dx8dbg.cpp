/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * D3DX8 debug-build tracing helpers, compiled only when DBG is set.
 * D3DXDebugPrintf formats a message, prefixes it with "D3DX: " and sends it to
 * the debugger; D3DXDebugPrintfHR appends the symbolic text of an HRESULT via
 * D3DXGetErrorStringA. D3DXDebugAssert is the assertion hook (a no-op here).
 * The DPF/DPFHR/D3DXASSERT macros expand to these in debug builds.
 */

#include "pchcore.h"
#if DBG


//
// DPF
//

void cdecl D3DXDebugPrintf(UINT lvl, LPSTR szFormat, ...)
{
    char strA[256];
    char strB[256];

//    if(lvl > (UINT) g_dwDebugLevel)
//        return;

    va_list ap;
    va_start(ap, szFormat);
    _vsnprintf(strA, sizeof(strA), szFormat, ap);
    strA[255] = '\0';
    va_end(ap);

    _snprintf(strB, sizeof(strB), "D3DX: %s\r\n", strA);
    strB[255] = '\0';

    OutputDebugStringA(strB);
}


//
// DPFHR
//

void cdecl D3DXDebugPrintfHR(UINT lvl, HRESULT hr, LPSTR szFormat, ...)
{
    char strA[256];
    char strB[256];

    va_list ap;
    va_start(ap, szFormat);
    _vsnprintf(strA, sizeof(strA), szFormat, ap);
    strA[255] = '\0';
    va_end(ap);

    D3DXGetErrorStringA(hr, strB, sizeof(strB));
    D3DXDebugPrintf(lvl, "%s: %s", strA, strB);
}



//
// D3DXASSERT
//

int WINAPI D3DXDebugAssert(LPCSTR szFile, int nLine, LPCSTR szCondition)
{
	return 0;
}


#endif // DBG