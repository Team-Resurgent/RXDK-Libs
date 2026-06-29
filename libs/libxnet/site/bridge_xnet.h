#pragma once
#define RXDK_XNET_BRIDGE_H

/*
 * Force-included before each libxnet TU's precomp.h. XNet is the Xbox TCP/IP
 * stack from private/ntos/xnet -- a KERNEL-runtime component: it pokes the MCPX
 * ethernet NIC, runs TCP timers via kernel DPC at DISPATCH_LEVEL, and calls
 * Ke/Mm/Io. So this bridge is shaped like libd3d8/libdsound (kernel runtime),
 * not a title-side lib. Built default-__cdecl; public APIs carry WINAPI/WSAAPI.
 */

/* XNet runs in the kernel runtime (like libdsound). */
#ifndef NTOS_KERNEL_RUNTIME
#define NTOS_KERNEL_RUNTIME 1
#endif
#ifndef _XBOX
#define _XBOX 1
#endif

/* Link xboxkrnl.lib directly: clear the dll-import decorations so the Ke/Mm/Io
   calls bind to the import library / libxapi facades rather than a thunk. */
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

/* MSVC CRT case-insensitive string compares (getxbyy.c / dns.c) -> picolibc. */
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif

/*
 * Establish the NT header environment in the proven order (nt -> ntrtl ->
 * nturtl -> ntos), same as libdsound's dscommon.h. Each xnet per-subdir
 * precomp.h includes <ntos.h> then <nturtl.h> but never <ntrtl.h>, so the
 * RTL_* types (RTL_CRITICAL_SECTION, RTL_DRIVE_LETTER_CURDIR, RTL_PROCESS_*)
 * that nturtl_sdk.h/winbase.h reference would be undefined. Pre-including here
 * (before the TU's precomp) fixes the order; the precomp's own includes are
 * then guarded no-ops. nt.h also defines NT_INCLUDED so windef.h skips zig's
 * MinGW winnt.h.
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>

/*
 * Internal Ethernet link-state flags used by phy.c / xnic.c / i82558.c. The
 * private header that defined these was dropped from the leak; they mirror the
 * public XNET_ETHERNET_LINK_* values (winsockx.h) bit-for-bit -- phy.c builds
 * exactly the value XnetGetEthernetLinkStatus() returns.
 */
#ifndef XNET_LINK_IS_UP
#define XNET_LINK_IS_UP        0x01  /* == XNET_ETHERNET_LINK_ACTIVE */
#define XNET_LINK_100MBPS      0x02
#define XNET_LINK_10MBPS       0x04
#define XNET_LINK_FULL_DUPLEX  0x08
#define XNET_LINK_HALF_DUPLEX  0x10
#endif
