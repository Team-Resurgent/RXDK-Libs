/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_XONLINE_BRIDGE_H

/*
 * Force-included before each libxonline TU (which then #includes "xonp.h", the
 * client's precompiled header). This is the Xbox Live client from private/online:
 * logon/presence/accounts/billing/match/stats/users/service, content download +
 * patching, the XDK's own crypto (Kerberos/MD5/ASN.1) and LZX decompressor. It is
 * a TITLE-SIDE library that sits directly on top of RXDK's libxnet (winsock/XNet),
 * libkernel, libxapi and libc. Built as the retail title variant
 * (XONLINE_BUILD_LIBX) -- see private/online/xonp.h / sources.inc.
 *
 * Xbox Live servers are long dead: this only needs to compile + link.
 *
 * The client uses kernel-native APIs directly (NtCreateFile/NtFsControlFile/
 * IoCreateSymbolicLink/ExQueryNonVolatileSetting/RtlInitObjectString) AND the
 * title surface (xtl/xapip Win32) AND libxnet's winsock/XNet headers. To get all
 * three consistently -- and identically to how libxnet compiled the shared
 * winsockp.h/xcrypt.h/xn.h -- this bridge establishes the SAME environment as
 * libxnet's xnp.h _XBOX block (ntos/nturtl/xtl/xboxp, NOT the xboxkrnl umbrella).
 * Using ntos.h rather than <xboxkrnl/xboxkrnl.h> is essential: the umbrella
 * redefines the Xc* crypto types (CRYPTO_VECTOR) and ExSaveNonVolatileSetting with
 * signatures that clash with the shared xcrypt.h / winsockp.h.
 *
 *   1. Build identity + features (NT, _XBOX, XONLINE_BUILD_LIBX, DBG=0,
 *      DASH_UPDATE_IN_TITLE) + i386 machine defines BEFORE any header.
 *   2. Clear dllimport decorations so Ke/Mm/Io/Nt/Ex/Xc/net calls bind to the RXDK
 *      import libraries rather than __imp_ thunks.
 *   3. Calling-convention + COM macros the slimmed headers expect.
 *   4. Establish the NT/kernel/title environment (ntos -> nturtl -> xtl -> xboxp),
 *      exactly as libxnet does, before xonp.h's own includes (which then become
 *      guarded no-ops).
 *   5. MSVC-CRT string/float shims + a couple of NT helpers -> picolibc / kernel.
 */

/* (1) Build identity + features (mirror private/online/sources.inc + xonp.h). */
#ifndef NT
#define NT 1
#endif
#ifndef _XBOX
#define _XBOX 1
#endif
#ifndef XBOX
#define XBOX 1
#endif
#ifndef XONLINE_BUILD_LIBX
#define XONLINE_BUILD_LIBX 1
#endif
#ifndef DASH_UPDATE_IN_TITLE
#define DASH_UPDATE_IN_TITLE 1
#endif
#ifndef DBG
#define DBG 0
#endif

/* i386 machine identity + NT-header selection (mirror libxapi compile.h essentials
   WITHOUT pulling the xboxkrnl umbrella). NT_INCLUDED makes windef skip zig's
   MinGW winnt.h and defer to the NT headers below. */
#ifndef _X86_
#define _X86_ 1
#endif
#ifndef i386
#define i386 1
#endif
#ifndef _M_IX86
#define _M_IX86 600
#endif
#define NT_UP 1
#define STD_CALL 1
#ifndef _XAPI_
#define _XAPI_ 1
#endif
/* NT_INCLUDED: makes shared/include/windef.h (reached via xtl.h) skip <winnt.h>
   and take its NT base types from the NT headers below rather than the xboxkrnl
   umbrella (whose LARGE_INTEGER/CONTEXT/etc. clash with ntos.h's). */
#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

/* (2) Bind kernel/net/crypto imports directly to the RXDK import libraries (no
   __declspec(dllimport) thunks) -- clear the decorations before any header. */
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
#ifdef XBAPI
#undef XBAPI
#endif
#define XBAPI

/* (3) Calling-convention macros the slimmed windef.h / winsock headers expect. */
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef WINAPIV
#define WINAPIV __cdecl
#endif
#ifndef WSAAPI
#define WSAAPI __stdcall
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef FASTCALL
#define FASTCALL __fastcall
#endif
#ifndef CDECL
#define CDECL __cdecl
#endif

/* COM calling-convention + declaration macros (ntdef.h only defines them under
   _WIN32). The crypto/net headers use STDMETHODCALLTYPE etc. */
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

/* (4) NT/kernel/title environment -- the SAME set as libxnet's xnp.h _XBOX block
   (ntos/init/hal/nturtl/xtl/xboxp), so the shared libxnet headers (winsockp.h,
   xcrypt.h, xn.h) compile here exactly as they did in libxnet. xonp.h's own
   re-includes of xtl/xapip then become guarded no-ops. Wrapped extern "C" for the
   C++ TUs. */
#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <init.h>
#include <hal.h>
#include <xtl.h>
#include <xboxp.h>
#ifdef __cplusplus
}
#endif

/* RtlEqualMemory: the standard NT macro (RtlCompareMemory == length). xonp.h's
   inline VerifyOnlineUserSignature + several TUs use it; the RXDK ntrtl subset
   omits the macro. */
#ifndef RtlEqualMemory
#define RtlEqualMemory(dst, src, len) (memcmp((dst), (src), (len)) == 0)
#endif

/* (5) MSVC CRT string/float intrinsics by their underscore names (picolibc C
   names). */
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#ifndef _snprintf
#define _snprintf  snprintf
#endif

/* Wide-string helpers the client uses (xbosutil/msasn1 format prices + tags).
   picolibc supplies wcslen/wcscpy; its swprintf is the C99 sized form, so remap
   the MSVC no-size swprintf(dst, fmt, ...) onto it with an effectively-unbounded
   count (compile+link only -- Xbox Live is dead, this never runs). The token
   `swprintf` inside the macro body is not re-expanded (C self-reference rule), so
   it binds to the real picolibc function. */
#include <wchar.h>
#ifndef swprintf
#define swprintf(dst, ...) swprintf((dst), 0x7fffffff, __VA_ARGS__)
#endif

/* _atoi64: MSVC CRT 64-bit parse -> picolibc atoll (both return long long). */
#ifndef _atoi64
#define _atoi64(s) atoll(s)
#endif

/* Force RETAIL: the client's #if DBG paths pull trace/assert diagnostics we don't
   build. */
#undef _DEBUG
#undef DEBUG
