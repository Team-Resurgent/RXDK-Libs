#pragma once
#define RXDK_D3DX8_BRIDGE_H

/*
 * Force-included (before each component's pch*.h) for every libd3dx8 translation
 * unit. d3dx8.lib is a TITLE-SIDE helper library: unlike libd3d8 (the kernel
 * NV2A driver), it links into a title alongside libxapi and calls the public
 * d3d8 + CRT surface. It is built default-__cdecl (d3dx.mk sets 386_STDCALL=0),
 * so -fdefault-calling-conv is NOT used here.
 *
 * Two gaps between the leak build env and ours, both mirroring bridge_d3d8.h:
 *  1. The component pch*.h files pull <xtl.h>, whose windef.h wants the Win32
 *     base/NT types from winnt.h. We must skip zig's MinGW winnt.h (NT_INCLUDED)
 *     and supply those types from xboxkrnl. libxapi's compile.h is exactly that
 *     title-build environment, so we reuse it.
 *  2. The leak's real xtl.h pulled in the whole DirectX surface, including the
 *     public d3dx8 headers; our libxapi xtl.h shim does not. The component pches
 *     (e.g. math/pchmath.h -> cstack.h) reference D3DXMATRIX / ID3DXMatrixStack
 *     without including d3dx8math.h, so we pre-include the <d3dx8.h> umbrella.
 */

/* (1) NT/title build environment: defines NT_INCLUDED (windef skips winnt.h),
   pulls xboxkrnl types (MEMORY_BASIC_INFORMATION etc. that winbase.h needs) and
   win32_bridge.h base types (DWORD/BYTE/UINT/...). */
#include "compile.h"

/* WINAPI/FASTCALL/HWND aren't in our slimmed windef.h. d3d8.h/d3dx8 public
   headers use WINAPI on every entry point and HWND in vestigial window fields.
   Use the MSVC calling-convention KEYWORDS (clang accepts them under
   -fms-extensions) so they bind in function-pointer typedefs and method decls. */
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
/* HMODULE: dxfile.h's DXFILELOADRESOURCE carries one. Not in our windef.h. */
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif
/* HINSTANCE: mesh/loadx.cpp's vestigial resource-load signatures use it. */
#ifndef _HINSTANCE_DEFINED_
#define _HINSTANCE_DEFINED_
DECLARE_HANDLE(HINSTANCE);
#endif
/* TCHAR: not UNICODE here, so the ANSI char form (savemesh.cpp uses TCHAR**). */
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif

/* COM method calling conventions. xobjbase.h (pulled by d3dx8.h) builds its COM
   vtables with STDMETHODCALLTYPE but doesn't define it; ntdef.h only defines
   these under _WIN32, which we don't target. Supply the standard i386 forms. */
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
/* STDAPI/STDAPI_: dxfile.h declares DirectXFileCreate with STDAPI. ntdef.h only
   defines these under _WIN32. */
#ifndef STDAPI
#define STDAPI             EXTERN_C HRESULT STDAPICALLTYPE
#endif
#ifndef STDAPI_
#define STDAPI_(type)      EXTERN_C type STDAPICALLTYPE
#endif
/* STDMETHODIMP/_: the d3dx8 COM object method definitions (CD3DXBuffer etc.)
   use these. ntdef.h only defines them under _WIN32. */
#ifndef STDMETHODIMP
#define STDMETHODIMP       HRESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#endif

/* Fixed-width Win32 types. These normally come from basetsd.h, which winnt.h
   pulls -- but NT_INCLUDED skips winnt.h, so basetsd.h is skipped too. The
   D3DX code (e.g. tex/pchtex.h's F2I FPU helpers) uses UINT32; supply the set. */
#ifndef _RXDK_D3DX8_FIXEDWIDTH
#define _RXDK_D3DX8_FIXEDWIDTH
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

/* GUID/REFIID/REFGUID for d3d8.h's resource interfaces and the d3dx8 COM
   objects (ID3DXBuffer/ID3DXSprite/...). Our xtl.h shim doesn't pull guiddef. */
#include <guiddef.h>

/* MAKEFOURCC: used by tex (DDS FourCC) and the effect parser. Normally in
   mmsystem.h/d3d; not in our slimmed headers. */
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif

/* _MAX_PATH: MSVC CRT path-length constant (stdlib.h). core/CD3DXFile uses it. */
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

/* Enable the inline D3DDevice::BeginStateBlock/EndStateBlock surface in d3d8.h.
   d3d8.h gates these behind D3DCOMPILE_BEGINSTATEBLOCK (defined before xtl.h);
   d3dx8's sprite / render-to-surface / env-map code records state blocks. */
#ifndef D3DCOMPILE_BEGINSTATEBLOCK
#define D3DCOMPILE_BEGINSTATEBLOCK 1
#endif

/* (2) Title surface then the D3DX PRIVATE umbrella. We pull d3dx8p.h (inc/),
   NOT the public d3dx8.h: d3dx8p.h includes the private d3dx8meshp.h, whose
   enum _D3DXMESHOPT carries the internal flags the mesh optimizer needs
   (D3DXMESHOPT_VALIDBITS etc.). Public d3dx8mesh.h and private d3dx8meshp.h
   share the guard __D3DX8MESH_H__, so whichever is included first wins -- and
   the components were built against the private superset. d3dx8p.h pulls
   <d3d8.h> + <xobjbase.h> + all the d3dx8 sub-headers; the component pch*.h
   re-includes are then guarded no-ops. d3dx8 uses the INLINE public d3d8 API
   (like the samples), so D3DCOMPILE_NOTINLINE is intentionally NOT defined. */
#include <xtl.h>
#include <d3dx8p.h>

/* ctype: xof6's parser uses isdigit/isxdigit/isspace; the leak pulled ctype via
   the title CRT headers, our shim doesn't. */
#include <ctype.h>

/* MSVC CRT float intrinsics the D3DX math/mesh code calls by their underscore
   names. picolibc exposes the C99 spellings; map the ones d3dx8 uses. */
#include <math.h>
#ifndef _finite
#define _finite(x) isfinite(x)
#endif
#ifndef _isnan
#define _isnan(x)  isnan(x)
#endif

/* MSVC CRT string/printf intrinsics by their underscore names. picolibc exposes
   the unprefixed C names. */
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#ifndef _snprintf
#define _snprintf  snprintf
#endif
/* MSVC CRT string intrinsics -> picolibc C names (strings.h = strcasecmp). */
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif
#ifndef _strdup
#define _strdup    strdup
#endif

/*
 * _controlfp + precision-control constants. mesh/simplify.inl forces 53-bit
 * (double) x87 precision around the simplification math via
 * _controlfp(_PC_53, _MCW_PC). picolibc has no _controlfp; implement it against
 * the x87 control word (PC field = bits 8-9). Only the PC mask is handled --
 * that's all the D3DX code uses. Faithful to the original intent.
 */
#ifndef _MCW_PC
#define _MCW_PC 0x00030000u   /* precision-control mask (MSVC abstract bits) */
#define _PC_24  0x00020000u   /* 24-bit (single)  */
#define _PC_53  0x00010000u   /* 53-bit (double)  */
#define _PC_64  0x00000000u   /* 64-bit (extended)*/
static __inline unsigned int __rxdk_controlfp(unsigned int newval, unsigned int mask)
{
    unsigned short cw;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    if (mask & _MCW_PC) {
        unsigned short pc;
        switch (newval & _MCW_PC) {
            case _PC_24: pc = 0x0000; break;  /* x87 PC=00 */
            case _PC_53: pc = 0x0200; break;  /* x87 PC=10 */
            default:     pc = 0x0300; break;  /* x87 PC=11 (_PC_64) */
        }
        cw = (unsigned short)((cw & ~0x0300) | pc);
        __asm__ __volatile__("fldcw %0" : : "m"(cw));
    }
    switch (cw & 0x0300) {                    /* report current PC in MSVC bits */
        case 0x0000: return _PC_24;
        case 0x0200: return _PC_53;
        default:     return _PC_64;
    }
}
#define _controlfp(newval, mask) __rxdk_controlfp((newval), (mask))
#endif

/*
 * Force RETAIL. d3dx8.lib (FREEBUILD) is the non-debug target. profile.h does
 * `#define DBG 0`, but several D3DX headers test `defined(DBG)` rather than its
 * value (e.g. mesh/common.h's `#if defined(DBG)||defined(DEBUG)||defined(_DEBUG)`)
 * and would otherwise switch on the debug path -- which pulls MinGW <crtdbg.h>.
 * Undefine all three right before the component pch*.h is parsed; d3dx8dbg.h and
 * dpf.h use value-based `#if DBG` and degrade correctly to their retail branch. */
#undef DBG
#undef _DEBUG
#undef DEBUG
