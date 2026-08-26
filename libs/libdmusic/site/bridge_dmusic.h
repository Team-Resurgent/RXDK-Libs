/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_DMUSIC_BRIDGE_H

/*
 * Force-included before each libdmusic translation unit. dmusic.lib is the
 * TITLE-SIDE DirectMusic runtime ported from the May-2020 leak
 * (private/windows/directx/dmusic): the interactive-music engine (dmime), the
 * software synthesizer (dmsynth), the loader (dmloader) and the authored-content
 * components (dmband/dmstyle/dmcompos/dmscript). It is COM-heavy and outputs
 * audio through DirectSound's PUBLIC API (IDirectSoundBuffer/IDirectSound), on
 * top of the xboxkrnl timer/DPC/IRQL + pool primitives (Ke.. and Ex..). It links
 * into a title alongside libxapi, libdsound and libc.
 *
 * Same shape as libxact's site/bridge_xact.h:
 *  1. Reuse libxapi compile.h for the NT/title build env (NT_INCLUDED so windef
 *     skips zig's MinGW winnt.h; XBOX=1 -- xtl.h is selected via XBOX). Pulls
 *     xboxkrnl (KTIMER/KDPC, KeGetCurrentIrql/PASSIVE_LEVEL, ExAllocatePool...).
 *  2. Supply the Win32/COM macros our slimmed headers lack, then pull the title
 *     umbrella DirectMusic leans on: <xtl.h> + <xobjbase.h> (Xbox COM machinery)
 *     + <dsound.h> (every DirectSound type dmusic names).
 *  3. ASSERT/RIP fallbacks + MSVC-CRT shims + retail-build forcing.
 */

/* (1) NT/title build environment (NT_INCLUDED, XBOX, xboxkrnl + win32_bridge types). */
#include "compile.h"

/* WINAPI/FASTCALL/handles not in our slimmed windef.h. */
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef WINAPIV
#define WINAPIV __cdecl
#endif
#ifndef APIENTRY
#define APIENTRY WINAPI
#endif
#ifndef FASTCALL
#define FASTCALL __fastcall
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef _HWND_DEFINED_
#define _HWND_DEFINED_
DECLARE_HANDLE(HWND);
#endif
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif
/* GDI/USER/MMSYSTEM handle types the slimmed windef.h lacks. mmsystem.h (the
   full multimedia header dmsynth/dmime pull) names HTASK/HDC/HDROP/HICON in its
   MCI + waveform structs; DirectMusic never uses those paths but the decls must
   parse. */
#ifndef _HDC_DEFINED_
#define _HDC_DEFINED_
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HDROP);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HICON);
#endif

/* COM calling-convention macros (ntdef.h only defines them under _WIN32). */
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE  __stdcall
#endif
#ifndef STDMETHODVCALLTYPE
#define STDMETHODVCALLTYPE __cdecl
#endif
#ifndef STDAPICALLTYPE
#define STDAPICALLTYPE     __stdcall
#endif
#ifndef STDAPIVCALLTYPE
#define STDAPIVCALLTYPE    __cdecl
#endif
#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif
#ifndef STDAPI
#define STDAPI             EXTERN_C HRESULT STDAPICALLTYPE
#endif
#ifndef STDAPI_
#define STDAPI_(type)      EXTERN_C type STDAPICALLTYPE
#endif
#ifndef STDMETHODIMP
#define STDMETHODIMP       HRESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#endif
#ifndef STDMETHOD
#define STDMETHOD(method)        virtual HRESULT STDMETHODCALLTYPE method
#endif
#ifndef STDMETHOD_
#define STDMETHOD_(type, method) virtual type STDMETHODCALLTYPE method
#endif

/* GUID/REFIID for the COM-style decls. */
#include <guiddef.h>

/* HIGH_LEVEL: top IRQL (dsynth/debug sizing). x86 IRQL max is 31. */
#ifndef HIGH_LEVEL
#define HIGH_LEVEL 31
#endif

/* MAXULONG_PTR: the perf/debug pool sizing uses it; our ntdef doesn't define it. */
#ifndef MAXULONG_PTR
#define MAXULONG_PTR (~((ULONG_PTR)0))
#endif

/* (2) The title umbrella DirectMusic leans on. xtl.h pulls the Win32/xapi surface
   (CRITICAL_SECTION, HANDLE, EnterCriticalSection ...); <xobjbase.h> supplies the
   Xbox COM interface machinery (DECLARE_INTERFACE/THIS/PURE/LPUNKNOWN) dsound.h's
   vtables need; <d3dx8math.h> supplies the D3DXVECTOR types the DS3D structs use;
   then the PUBLIC <dsound.h> for every DirectSound type dmusic references. */
#include <xtl.h>
#include <xobjbase.h>
#include <d3dx8math.h>
#include <dsound.h>

/* The libxapi NT/RTL headers redefine try/except/finally/leave as SEH stubs
   (try -> if(1)) for the kernel C code. DirectMusic is C++ and uses genuine
   C++ try/catch (defensive catch(...) around throwing new). Undo those macros so
   the component TUs get real exception syntax -- they never use SEH themselves. */
#ifdef try
#undef try
#endif
#ifdef except
#undef except
#endif
#ifdef finally
#undef finally
#endif
#ifdef leave
#undef leave
#endif

/* std::min/max/swap: DirectMusic uses these bare in shared/miscutil.h and via
   each component pch's <xutility>. Provide the minimal set for every C++ TU
   (our inc/xutility). C++-only so the one C TU (medparam_i.c) is unaffected. */
#ifdef __cplusplus
#include <xutility>
#endif

/* ASSERT / RIP fallbacks (each component's debug.h #undef's + redefines these per
   the DBG level; these are only the pre-debug.h fallbacks). */
#ifndef ASSERT
#define ASSERT(x)   ((void)0)
#endif
/* variadic: the standard ASSERTMSG is ASSERTMSG(msg, expr). */
#ifndef ASSERTMSG
#define ASSERTMSG(...) ((void)0)
#endif

/* MSVC CRT float/string intrinsics by their underscore names (picolibc C names). */
#include <math.h>
#ifndef _finite
#define _finite(x) isfinite(x)
#endif
#ifndef _isnan
#define _isnan(x)  isnan(x)
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#ifndef _snprintf
#define _snprintf  snprintf
#endif
#include <ctype.h>   /* tolower/toupper/isspace ... (dmscript lexer) */
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif
/* MSVC integer-limit macros the param tracks use (picolibc's <stdint.h> spells
   them INT64_MIN etc.). */
#include <stdint.h>
#ifndef _I64_MIN
#define _I64_MIN  INT64_MIN
#endif
#ifndef _I64_MAX
#define _I64_MAX  INT64_MAX
#endif
#ifndef _UI64_MAX
#define _UI64_MAX UINT64_MAX
#endif
#ifndef _I32_MIN
#define _I32_MIN  INT32_MIN
#endif
#ifndef _I32_MAX
#define _I32_MAX  INT32_MAX
#endif

/* Wide-string CRT (wcscpy/wcslen/wcscmp ...): the loader + track code manipulates
   the WCHAR object names. picolibc supplies these in <wchar.h>. */
#include <wchar.h>
#ifndef _wcsicmp
#define _wcsicmp   wcscasecmp
#endif
#ifndef _wcsnicmp
#define _wcsnicmp  wcsncasecmp
#endif

/* MSVC CRT helpers the scripting engine uses that picolibc doesn't spell:
   _Pow_int (STL integer power) and _ultow (unsigned-long -> wide string). */
#ifdef __cplusplus
static __inline long _Pow_int(long base, long exp)
{
    long result = 1;
    if (exp < 0) return 0;
    while (exp-- > 0) result *= base;
    return result;
}
#endif
static __inline wchar_t *_ultow(unsigned long value, wchar_t *str, int radix)
{
    wchar_t tmp[33]; int i = 0;
    if (radix < 2 || radix > 36) { str[0] = 0; return str; }
    if (value == 0) tmp[i++] = L'0';
    while (value) { unsigned d = (unsigned)(value % (unsigned)radix); tmp[i++] = (wchar_t)(d < 10 ? L'0' + d : L'a' + d - 10); value /= (unsigned)radix; }
    int j = 0; while (i > 0) str[j++] = tmp[--i]; str[j] = 0;
    return str;
}

/* Force RETAIL: the components' #if DBG paths pull DirectMusic debug diagnostics
   (DbgPrint traces + debug-context globals) not built for playback. */
#undef DBG
#undef _DEBUG
#undef DEBUG
