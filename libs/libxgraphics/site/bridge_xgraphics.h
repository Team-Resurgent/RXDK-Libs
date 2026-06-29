#pragma once
#define RXDK_XGRAPHICS_BRIDGE_H

/*
 * Force-included (before each component's pch / direct includes) for every
 * libxgraphics translation unit. xgraphics.lib is a TITLE-SIDE helper library
 * (swizzle/format helpers, S3TC, the shader assembler, xgmath) that links into a
 * title alongside libxapi and the public d3d8 surface. Built default-__cdecl.
 *
 * Same shape as libd3dx8's site/bridge_d3dx8.h:
 *  1. Reuse libxapi compile.h for the NT/title build env (defines NT_INCLUDED so
 *     windef skips zig's MinGW winnt.h, and XBOX=1 -- the components select their
 *     <xtl.h> include via `#ifdef XBOX`; the #else path is win32-only).
 *  2. Supply the Win32/COM macros + handle/fixed-width types our slimmed headers
 *     lack, then pull <d3d8.h> (D3DFORMAT etc. that <xgraphics.h> uses but does
 *     not include -- the real XDK xtl.h pulled the D3D surface) and <xgraphics.h>.
 *  3. MSVC-CRT shims and retail-build forcing, as for d3dx8.
 */

/* (1) NT/title build environment (NT_INCLUDED, XBOX, xboxkrnl + win32_bridge types). */
#include "compile.h"

/* WINAPI/FASTCALL/handles not in our slimmed windef.h. */
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

/* COM calling-convention macros (ntdef.h only defines them under _WIN32). The
   shadeasm shader-validator interfaces and any COM-style decls need them. */
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

/* Fixed-width Win32 types (basetsd.h is skipped along with winnt.h). */
#ifndef _RXDK_XGRAPHICS_FIXEDWIDTH
#define _RXDK_XGRAPHICS_FIXEDWIDTH
typedef signed char        INT8;
typedef short              INT16;
typedef int                INT32;
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef long               LONG32;
typedef unsigned long      ULONG32;
typedef unsigned int       DWORD32;
#endif

/* GUID/REFIID for any COM-style decls. */
#include <guiddef.h>

/* MAKEFOURCC: dxtc/swizzler use D3DFMT FourCC codes. */
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

/* (2) Title surface + the D3D types <xgraphics.h> relies on, then xgraphics.h.
   xgraphics.h declares WINAPI helpers over D3DFORMAT/D3DSURFACE etc. but includes
   nothing itself (the real xtl.h pulled the D3D surface), so pre-pull <d3d8.h>. */
#include <xtl.h>
#include <d3d8.h>
#include <xgraphics.h>

/* ctype: the shadeasm tokenizer uses isspace/isalpha/isalnum/isdigit. */
#include <ctype.h>

/* MSVC CRT float intrinsics by their underscore names. */
#include <math.h>
#ifndef _finite
#define _finite(x) isfinite(x)
#endif
#ifndef _isnan
#define _isnan(x)  isnan(x)
#endif

/* MSVC CRT string/printf intrinsics -> picolibc C names. */
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#ifndef _snprintf
#define _snprintf  snprintf
#endif
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif
#ifndef _itoa
#define _itoa      itoa   /* shadeasm/preprocessor.cpp; picolibc provides itoa */
#endif

/* Force RETAIL (see bridge_d3dx8.h -- several headers test defined(DBG)). */
#undef DBG
#undef _DEBUG
#undef DEBUG
