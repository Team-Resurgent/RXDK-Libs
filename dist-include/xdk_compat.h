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
// (The remaining MSVC CRT shims -- _stricmp/_controlfp/_fpclass -- live in the
// bridges only: they are used by library .cpp sources, never by the public
// headers. _finite/_isnan used to be in that group, but a title does reach for
// them -- TechCertGame's physics guards every result with _finite -- so they are
// part of the title surface below.)
//

/*
 * MSVC CRT floating-point predicates. The XDK CRT had these; picolibc spells them
 * isfinite/isnan. Written against the compiler builtins rather than <math.h> so
 * this header stays include-free, and guarded like everything else here.
 */
#ifndef _finite
#define _finite(x) (__builtin_isfinite(x))
#endif
#ifndef _isnan
#define _isnan(x)  (__builtin_isnan(x))
#endif

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

// MSVC CRT case-insensitive string compares. picolibc provides the POSIX names
// (strcasecmp / strncasecmp, in <strings.h>); a lot of legacy Windows/Xbox code uses the
// MSVC spellings without the leading underscore. Map the common ones so ported titles build.
#include <strings.h>
#ifndef stricmp
#define stricmp   strcasecmp
#endif
#ifndef strnicmp
#define strnicmp  strncasecmp
#endif
#ifndef strcmpi
#define strcmpi   strcasecmp
#endif
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp strncasecmp
#endif
// Wide equivalents (picolibc supplies wcscasecmp / wcsncasecmp in <wchar.h>).
#include <wchar.h>
#ifndef _wcsicmp
#define _wcsicmp  wcscasecmp
#endif
#ifndef wcsicmp
#define wcsicmp   wcscasecmp
#endif
#ifndef wcsnicmp
#define wcsnicmp  wcsncasecmp   /* _wcsnicmp is defined further down */
#endif
#include <stdarg.h> // va_list, for the _vsnprintf/_vscprintf declarations below
#include <stddef.h> // size_t
// MSVC printf-family spellings. These are real functions (libs/libxapi/port/
// compat.c), NOT aliases for snprintf/vsnprintf: the size argument means
// something different. C99 treats it as the buffer size including the NUL and
// always terminates; MSVC treats it as the maximum number of characters to
// write and only terminates when the result fits in fewer. Aliasing them made
// the common Xbox idiom
//     len = _vsnprintf(NULL, 0, fmt, ap);
//     _vsnprintf(buf, len, fmt, ap); buf[len] = 0;
// drop its last character.
//
// The extern "C" matters: these are defined in a C translation unit, so a C++
// title that saw them with C++ linkage would fail to link against the mangled
// name.
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _snprintf
int _snprintf(char *buffer, size_t count, const char *format, ...);
#endif
#ifndef _vsnprintf
int _vsnprintf(char *buffer, size_t count, const char *format, va_list ap);
#endif
// MSVC's portable way to ask how long a formatted string would be.
#ifndef _scprintf
int _scprintf(const char *format, ...);
#endif
#ifndef _vscprintf
int _vscprintf(const char *format, va_list ap);
#endif
#ifdef __cplusplus
}
#endif

// NT RtlXxxMemory intrinsics -- XDK samples use these; the picolibc/xtl umbrella doesn't supply them.
#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#endif
#ifndef RtlMoveMemory
#define RtlMoveMemory(Destination, Source, Length) memmove((Destination), (Source), (Length))
#endif
#ifndef RtlFillMemory
#define RtlFillMemory(Destination, Length, Fill) memset((Destination), (Fill), (Length))
#endif
#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

// MSVC CRT spellings that alias directly onto the picolibc/POSIX equivalents.
#ifndef _fcvt
#define _fcvt fcvt
#endif
#ifndef _itoa
#define _itoa itoa
#endif
#ifndef _wcsnicmp
#define _wcsnicmp wcsncasecmp
#endif

// MSVC wide CRT helpers the samples call. _snwprintf/_vsnwprintf map to C99 vswprintf (same arg
// shape); _itow/_i64tow are small base-radix conversions (picolibc has no _itow/_i64tow).
#ifdef __cplusplus
#include <wchar.h>  // vswprintf
#ifndef _vsnwprintf
static inline int _vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list ap)
{
    return vswprintf(buffer, count, format, ap);
}
#endif
#ifndef _i64tow
static inline wchar_t *_i64tow(long long value, wchar_t *str, int radix)
{
    wchar_t *p = str, *lo, *hi;
    unsigned long long uv = (radix == 10 && value < 0) ? (*p++ = L'-', (unsigned long long)(-value)) : (unsigned long long)value;
    lo = p;
    do { unsigned d = (unsigned)(uv % (unsigned)radix); *p++ = (wchar_t)(d < 10 ? L'0' + d : L'a' + d - 10); uv /= (unsigned)radix; } while (uv);
    *p = 0;
    for (hi = p - 1; lo < hi; ++lo, --hi) { wchar_t t = *lo; *lo = *hi; *hi = t; }
    return str;
}
#endif
#ifndef _snwprintf
static inline int _snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...)
{
    va_list ap; int r;
    va_start(ap, format);
    r = vswprintf(buffer, count, format, ap);
    va_end(ap);
    return r;
}
#endif
#ifndef _wtol
static inline long _wtol(const wchar_t *str)
{
    return wcstol(str, (wchar_t **)0, 10);
}
#endif
#ifndef _itow
static inline wchar_t *_itow(int value, wchar_t *str, int radix)
{
    wchar_t *p = str, *lo, *hi;
    unsigned int uv = (radix == 10 && value < 0) ? (*p++ = L'-', (unsigned)(-value)) : (unsigned)value;
    lo = p;
    do { unsigned d = uv % (unsigned)radix; *p++ = (wchar_t)(d < 10 ? L'0' + d : L'a' + d - 10); uv /= (unsigned)radix; } while (uv);
    *p = 0;
    for (hi = p - 1; lo < hi; ++lo, --hi) { wchar_t t = *lo; *lo = *hi; *hi = t; }
    return str;
}
#endif
#endif // __cplusplus

// MSVC aligned allocation -> C11 aligned_alloc (note the swapped argument order) + free.
#ifndef _aligned_malloc
#define _aligned_malloc(size, alignment) aligned_alloc((alignment), (size))
#endif
#ifndef _aligned_free
#define _aligned_free free
#endif

// Legacy MSVC/XDK 2-argument swprintf(dst, fmt, ...) -- the pre-C99 form with no
// size argument. Picolibc declares only the C99 swprintf(dst, n, fmt, ...), so
// XDK title code that calls the 2-arg form (e.g. the sample framework's
// on-screen text: swprintf(buf, L"%.2f", v)) fails to compile. Provide a C++
// overload forwarding to vswprintf with an unbounded size. Overload resolution
// selects this only when the 2nd argument is a wide string (not a size_t), so
// the C99 3-arg form still binds to the real picolibc swprintf. C++ only: the
// C99 swprintf keeps C linkage, this overload has C++ linkage (a name may have
// at most one C-linkage function, which is satisfied).
#ifdef __cplusplus
#include <wchar.h>
#include <stdarg.h>
inline int swprintf(wchar_t *_Dst, const wchar_t *_Fmt, ...)
{
    va_list _Args;
    va_start(_Args, _Fmt);
    int _Ret = vswprintf(_Dst, (size_t)-1, _Fmt, _Args);
    va_end(_Args);
    return _Ret;
}
#endif

// D3D performance-instrumentation hook (XDK D3D8Perf.h, pulled implicitly by the
// XDK's d3d8.h). The XDK sample framework (Common/XBApp.cpp) calls it once per
// frame. This used to be an inline FALSE shim; the real export now lives in
// libd3d8/libd3d8i (whose retail plain-lib body is an unconditional FALSE), so
// this is just the declaration for translation units that never include
// d3d8perf.h -- and it must carry the same extern "C" linkage as that header's.
#if defined(__cplusplus) && !defined(_RXDK_D3DPERF_SHIM)
#define _RXDK_D3DPERF_SHIM
extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame(void);
#endif

// MAXULONG_PTR (basetsd.h) — largest ULONG_PTR value, used as a sentinel by a few
// XDK samples. ULONG_PTR is 32-bit on the Xbox target.
#ifndef MAXULONG_PTR
#define MAXULONG_PTR (~(ULONG_PTR)0)
#endif

// FIELD_OFFSET (winnt.h) — byte offset of a member. NT_INCLUDED skips winnt.h, so
// this never reached a title even though libxapi has it internally; GlobalFX uses
// it to tell SetEffectData which field of a DSP parameter block it is writing.
// Spelled with the builtin rather than winnt.h's &(((type *)0)->field): same
// value, but it is a constant expression the compiler blesses instead of a null
// dereference it warns about.
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG)__builtin_offsetof(type, field))
#endif
