/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_XNET_BRIDGE_H

/*
 * Force-included before each libxnet TU (which then #includes "xnp.h", the
 * stack's precompiled header). This is the Xbox kernel-runtime net stack: it
 * pokes the MCPX NIC, runs TCP timers via kernel DPC at DISPATCH_LEVEL, and
 * shares the NIC with xbdm through a native CXbdmServer/CXbdmClient. This header
 * sets up the build identity and header environment the stack expects.
 */

/* Build identity: Xbox target, retail (DBG=0), in one of TWO variants -- the same split the retail
   XDK ships, because the feature sets are not nested the way they look:

     XNET_BUILD_LIBX -> libxnet.lib   ARP/DHCP/DNS/FRAG/ICMP/INSECURE/ROUTE/XBDM_CLIENT/XBOX
     XNET_BUILD_LIBO -> libxneto.lib  the above plus ONLINE/QOS/SG

   LIBO looks like a superset, but it is not one in
   the sense that matters to a linker: XNET_FEATURE_SG compiles the secure-gateway Kerberos key
   exchange into ip.cpp, and that code calls CXoBase::XoKerb{GetInfo,BuildApReq,CrackApRep,
   GetAuthData} -- four symbols that live in libxonline. So a LIBO libxnet cannot be linked without
   libxonline even by a title that only wants a UDP socket. Titles pick: sockets-only links
   libxnet.lib, Xbox Live (incl. community revivals such as Insignia) links libxneto.lib +
   libxonline.lib, which is also what libxonline's own CXo layer is built against. */
#ifndef NT
#define NT 1
#endif
#ifndef _XBOX
#define _XBOX 1
#endif
#if !defined(XNET_BUILD_LIBX) && !defined(XNET_BUILD_LIBO)
#define XNET_BUILD_LIBO 1
#endif
#ifndef DBG
#define DBG 0
#endif

/* Link xboxkrnl.lib / libxapi facades directly: clear the dll-import decorations
   so Ke/Mm/Io/Hal calls bind to the import library rather than a thunk. */
#define RXDK_XNET_LINK 1
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
/* Some kernel prototypes reach libxnet via the shared xboxkrnl/api headers,
   which decorate with XBAPI (__declspec(dllimport)). Clear it too so those calls
   bind cdecl to the libxapi facades like everything else (no __imp_ thunks). */
#ifdef XBAPI
#undef XBAPI
#endif
#define XBAPI

/* Calling-convention macros the slimmed windef.h / winsock headers expect. */
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

/* MSVC CRT case-insensitive string compares -> picolibc. */
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif

/*
 * Establish the NT header environment in the proven order (nt -> ntrtl ->
 * nturtl -> ntos) BEFORE xnp.h's own includes. xnp.h's _XBOX block includes
 * <nturtl.h> (and xtl.h -> winbase.h) but never <ntrtl.h>, so the RTL_* types
 * (RTL_CRITICAL_SECTION, RTL_DRIVE_LETTER_CURDIR, RTL_PROCESS_*) would be
 * undefined. Pre-including here fixes the order; xnp.h's re-includes become
 * guarded no-ops. Wrapped extern "C" for the C++ TUs (this bridge is also
 * force-included into the C tcpipxsum.c, where extern "C" is invalid -> guard).
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#ifdef __cplusplus
}
#endif
