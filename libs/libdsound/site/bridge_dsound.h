#pragma once
#define RXDK_DSOUND_BRIDGE_H

/*
 * Force-included before each libdsound TU's precomp (dsound/dsoundi.h ->
 * common/dscommon.h). dsound.lib is the Xbox DirectSound implementation -- the
 * MCPX APU hardware driver. Unlike d3dx8/xgraphics (pure title-side), dscommon.h
 * pulls the KERNEL header set under _XBOX (nt.h, ntrtl, nturtl, ntos.h, pci.h)
 * AND xtl.h: it runs in the title but pokes APU/PCI hardware and calls Av/Mm/Ke.
 * So this bridge is shaped like libd3d8's bridge_d3d8.h (kernel runtime), not the
 * title-side d3dx8 one. Built default-__cdecl (APIs carry explicit WINAPI).
 *
 * dscommon.h does its own NT includes (its `#include <nt.h>` defines NT_INCLUDED,
 * so windef skips zig's MinGW winnt.h). This bridge only needs to: mark the
 * kernel-runtime env so the Av/Mm/Ke calls bind to xboxkrnl's import lib, supply
 * the Win32/COM macros our slimmed headers lack, and add the MSVC-CRT shims.
 */

/* DirectSound runs in the kernel runtime (like libd3d8). */
#ifndef NTOS_KERNEL_RUNTIME
#define NTOS_KERNEL_RUNTIME 1
#endif
#ifndef _XBOX
#define _XBOX 1
#endif

/* libdsound links xboxkrnl.lib directly; clear the dll-import decorations so the
   kernel calls bind to the import library rather than a thunk. */
#define RXDK_DSOUND_LINK 1
#ifdef DECLSPEC_IMPORT
#undef DECLSPEC_IMPORT
#endif
#define DECLSPEC_IMPORT
#ifdef NTKERNELAPI
#undef NTKERNELAPI
#endif
#define NTKERNELAPI
#ifdef NTHALAPI
#undef NTHALAPI
#endif
#define NTHALAPI

/* WINAPI/FASTCALL/handles not in our slimmed windef.h. */
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef FASTCALL
#define FASTCALL __fastcall
#endif
/* HWND: vestigial in dsoundp.h. Plain typedef -- DECLARE_HANDLE isn't defined
   yet at bridge time (this bridge pre-includes nothing before the macros). */
#ifndef _HWND_DEFINED_
#define _HWND_DEFINED_
typedef void *HWND;
#endif
/* HMODULE: dxfile.h (pulled via d3dx8math.h -> d3dx8 umbrella, for D3DXVECTOR3)
   carries one. */
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
typedef void *HMODULE;
#endif

/* COM calling-convention macros (ntdef.h only defines them under _WIN32). The
   DirectSound COM interfaces (IDirectSound/IDirectSoundBuffer) need them. */
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

/* MSVC CRT intrinsics by their underscore names (picolibc C names). */
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
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif

/* MSVC _fpclass classification (dsmath.cpp). MSVC's <float.h> _fpclass returns
   one of these bits; picolibc has no _fpclass, so reimplement it from the C99
   fpclassify/signbit primitives and #define the class bits to the MSVC values. */
#ifndef _FPCLASS_SNAN
#define _FPCLASS_SNAN   0x0001  /* signaling NaN */
#define _FPCLASS_QNAN   0x0002  /* quiet NaN */
#define _FPCLASS_NINF   0x0004  /* negative infinity */
#define _FPCLASS_NNORM  0x0008  /* negative normal */
#define _FPCLASS_NDENORM 0x0010 /* negative denormal */
#define _FPCLASS_NZERO  0x0020  /* negative zero */
#define _FPCLASS_PZERO  0x0040  /* positive zero */
#define _FPCLASS_PDENORM 0x0080 /* positive denormal */
#define _FPCLASS_PNORM  0x0100  /* positive normal */
#define _FPCLASS_PINF   0x0200  /* positive infinity */
#endif
#ifndef RXDK_DSOUND_FPCLASS
#define RXDK_DSOUND_FPCLASS
static __inline int _fpclass(double __x)
{
    /* Use the clang builtins directly: picolibc's non-builtin signbit/fpclassify
       macros reference __signbit/__fpclassifyd which aren't linkable here. */
    int __neg = __builtin_signbit(__x);
    switch (__builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL,
                                 FP_SUBNORMAL, FP_ZERO, __x))
    {
    case FP_NAN:       return _FPCLASS_QNAN; /* can't distinguish S/Q NaN */
    case FP_INFINITE:  return __neg ? _FPCLASS_NINF : _FPCLASS_PINF;
    case FP_ZERO:      return __neg ? _FPCLASS_NZERO : _FPCLASS_PZERO;
    case FP_SUBNORMAL: return __neg ? _FPCLASS_NDENORM : _FPCLASS_PDENORM;
    default:           return __neg ? _FPCLASS_NNORM : _FPCLASS_PNORM;
    }
}
#endif
