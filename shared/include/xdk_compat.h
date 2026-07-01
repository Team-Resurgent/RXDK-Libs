#pragma once
//
// xdk_compat.h - RXDK distribution compatibility shim.
//
// The public headers (d3d8.h, d3dx8.h, dxfile.h, dsound.h, xmv.h, xobjbase.h,
// ...) reference a handful of Win32 names that, when building the libraries
// in-tree, are supplied by each library's private site/bridge_*.h force-
// include. Those bridge headers are NOT part of the distribution, so a title
// that includes the public headers from dist would miss these names.
//
// This header distills the *title-facing* subset of those bridges into the
// public distribution. It is pulled by <xtl.h> (after <windef.h>/<winbase.h>/
// <guiddef.h>, so DECLARE_HANDLE / DWORD / BYTE / HRESULT / EXTERN_C are
// available). Every definition is guarded, so it never conflicts with an
// in-tree bridge or a fuller Win32 header that already supplies the name.
//
// (The MSVC CRT function shims -- _finite/_isnan/_stricmp/_controlfp/_fpclass
// -- live in the bridges too, but are used only by library .cpp sources, never
// by the public headers, so they are intentionally left out of the title
// surface.)
//

/* Calling-convention keywords (clang honors them under -fms-extensions). */
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

/* Win32 handles the slimmed windef.h omits (d3d8/dsoundp: HWND; dxfile:
   HMODULE; d3dx8 mesh resource signatures: HINSTANCE). */
#ifndef _HWND_DEFINED_
#define _HWND_DEFINED_
DECLARE_HANDLE(HWND);
#endif
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif
#ifndef _HINSTANCE_DEFINED_
#define _HINSTANCE_DEFINED_
DECLARE_HANDLE(HINSTANCE);
#endif

/* ANSI TCHAR family (Xbox never builds UNICODE). dxfile.h uses LPCTSTR. */
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
typedef char _TCHAR;
#endif
#ifndef __LPCTSTR_DEFINED
#define __LPCTSTR_DEFINED
typedef char       *LPTSTR;
typedef const char *LPCTSTR;
#endif

/* COM calling-convention + entry macros. xobjbase.h self-supplies the first
   four as well (guarded, so both are safe); dxfile.h uses STDAPI. ntdef.h only
   defines these under _WIN32, which the Xbox target is not. */
#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE   __stdcall
#endif
#ifndef STDMETHODVCALLTYPE
#define STDMETHODVCALLTYPE  __cdecl
#endif
#ifndef STDAPICALLTYPE
#define STDAPICALLTYPE      __stdcall
#endif
#ifndef STDAPIVCALLTYPE
#define STDAPIVCALLTYPE     __cdecl
#endif
#ifndef STDAPI
#define STDAPI              EXTERN_C HRESULT STDAPICALLTYPE
#endif
#ifndef STDAPI_
#define STDAPI_(type)       EXTERN_C type STDAPICALLTYPE
#endif
#ifndef STDMETHODIMP
#define STDMETHODIMP        HRESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#endif

/* Fixed-width Win32 types. These normally come from basetsd.h, which winnt.h
   pulls -- but NT_INCLUDED skips winnt.h, so basetsd.h is skipped too. The
   public D3DX surface uses UINT32/etc. */
#ifndef _RXDK_FIXEDWIDTH_INTS
#define _RXDK_FIXEDWIDTH_INTS
typedef signed char    INT8;
typedef short          INT16;
typedef int            INT32;
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef long           LONG32;
typedef unsigned long  ULONG32;
typedef unsigned int   DWORD32;
#endif

/* Constants the public D3DX surface uses (normally mmsystem.h / MSVC stdlib.h). */
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
