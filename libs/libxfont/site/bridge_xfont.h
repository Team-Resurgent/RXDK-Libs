/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Force-included prefix header for the whole xfont library: it sets up the
 * NT/title build environment and the small set of Win32/COM/CRT aliases the
 * vendor XFONT sources expect but the RXDK slimmed headers do not provide.
 */

#pragma once
#define RXDK_XFONT_BRIDGE_H

/*
 * Force-included (before each libxfont TU's own includes) for the whole
 * library. xfont.lib is a TITLE-SIDE helper library, same shape as
 * libd3dx8 -- it calls the public d3d8.h surface + plain CRT, and makes no
 * direct xboxkrnl/NT calls (confirmed by inspection: no Av/Mm/Ke/Nt* calls
 * anywhere in xfont.c/bitmap.cpp/painttext.cpp). Unlike libd3d8/libdsound
 * (both /Gz kernel-runtime code, needing -fdefault-calling-conv=stdcall +a
 * cdecl_libc.h to pin bare CRT calls back to __cdecl), the vendor XFONT
 * source already tags every public entry point with an explicit __stdcall/
 * __fastcall, and its few untagged internal helpers/CRT calls
 * (malloc/free/memmove/ZeroMemory) are all self-consistent under the
 * compiler's plain i386 default (__cdecl) -- so this library, like
 * libd3dx8, does NOT set -fdefault-calling-conv=stdcall and needs no
 * cdecl_libc.h at all.
 */

/* NT/title build environment: defines NT_INCLUDED (windef skips winnt.h),
   pulls xboxkrnl types (MEMORY_BASIC_INFORMATION etc. that winbase.h needs)
   and win32_bridge.h base types (DWORD/BYTE/UINT/...). Same as libd3dx8. */
#include "compile.h"

/* WINAPI/FASTCALL/HWND/HMODULE aren't in our slimmed windef.h. d3d8.h (pulled
   below) tags every entry point with WINAPI; xfont.h itself uses __stdcall/
   __fastcall directly but forward-declares IDirect3DSurface8, so the WINAPI-
   tagged d3d8.h surface still needs these. */
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef FASTCALL
#define FASTCALL __fastcall
#endif
#ifndef _HWND_DEFINED_
#define _HWND_DEFINED_
DECLARE_HANDLE(HWND);
#endif
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif

/* COM method calling conventions. d3d8.h's resource interfaces build their
   vtables with these; ntdef.h only defines them under _WIN32. */
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

/* GUID/REFGUID: d3d8.h's resource interfaces (SetPrivateData etc.) reference
   them. Our xtl.h shim doesn't pull guiddef -- same gap bridge_d3dx8.h fills. */
#include <guiddef.h>

/* Title surface: xtl.h then the public d3d8.h (XFONT only needs D3DCOLOR/
   D3DFORMAT/D3DRECT/D3DSurface + the D3DSurface_LockRect/UnlockRect/GetDesc
   free functions -- no D3DX types, so no d3dx8 umbrella needed here). */
#include <xtl.h>
#include <d3d8.h>

/* MSVC CRT printf intrinsic by its underscore name (xfont.c's #if DBG-gated
   XFRIP helper calls _vsnprintf; DBG is 0 project-wide so this is dead code,
   but it still has to parse). picolibc exposes the unprefixed C name. */
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#ifndef _snprintf
#define _snprintf  snprintf
#endif

/*
 * _MAX_PATH: the MSVC CRT spelling, used by bitmap.cpp's disk-file font loader.
 * windef.h has MAX_PATH; the underscored alias lives in the title-facing
 * xdk_compat.h, which the libraries do not pull.
 */
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
