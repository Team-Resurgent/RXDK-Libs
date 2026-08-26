/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * D3DX8 debug-tracing interface. Declares the debug print/assert entry points
 * and the DPF/DPFHR/D3DXASSERT macros used across the library, which expand to
 * real calls in debug (DBG) builds and to no-ops otherwise. Also provides the
 * one-shot SURFACE_PORT_WARNING and optional memory-leak-tracking hooks.
 */


#ifndef __D3DX8DBG_H__
#define __D3DX8DBG_H__

#include <xdbg.h>

#if DBG
#define SURFACE_PORT_WARNING() \
{ \
    static BOOL bWarned = FALSE; \
    if (!bWarned) { \
        XDebugWarning("D3DX8", "Loading from resource is more efficient than using D3DX"); \
        XDebugWarning("D3DX8", "See bundler tool in XDK for examples on how to do it"); \
        bWarned = TRUE; \
    } \
}
#else
#define SURFACE_PORT_WARNING()
#endif

//
// DPF
//

#if DBG

    void cdecl D3DXDebugPrintf(UINT lvl, LPSTR szFormat, ...);
    void cdecl D3DXDebugPrintfHR(UINT lvl, HRESULT hr, LPSTR szFormat, ...);

    #define DPF D3DXDebugPrintf
    #define DPFHR D3DXDebugPrintfHR

#else // !DBG

    #pragma warning(disable:4002)
    /* RXDK: MSVC tolerated calling these 0-arg macros with arguments (warning
       C4002, args discarded); clang errors. Make them variadic no-ops. */
    #define DPF(...)
    #define DPFHR(...)

#endif // !DBG




//
// D3DXASSERT
//

#if DBG

    int WINAPI D3DXDebugAssert(LPCSTR szFile, int nLine, LPCSTR szCondition);

    #define D3DXASSERT(condition) \
        do { if(!(condition) && D3DXDebugAssert(__FILE__, __LINE__, #condition)) DebugBreak(); } while(0)

#else // !DBG

    #define D3DXASSERT(condition) 0

#endif // !DBG


//
// Memory leak checking
//
#ifdef DBG
//#define MEM_DEBUG
#endif

#ifdef MEM_DEBUG

    BOOL WINAPI D3DXDumpUnfreedMemoryInfo();
    void *operator new(size_t stAllocateBlock, const UINT32 uiLineNumber, const char *szFilename);
    void operator delete(void *pvMem, size_t stAllocateBlock, const UINT32 uiLineNumber, const char *szFilename);
    //void operator delete( void *pvMem );


#define new new(__LINE__, __FILE__)

#else // !DBG

    BOOL WINAPI D3DXDumpUnfreedMemoryInfo();

#define New new

#endif // !DBG

#endif // __D3DX8DBG_H__
