/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_XACT_BRIDGE_H

/*
 * Force-included before each libxact translation unit (engine/*.cpp -> stdafx.h/
 * xacti.h). xacteng.lib is the TITLE-SIDE XACT runtime audio engine. It drives
 * playback through DirectSound's PUBLIC API (IDirectSoundBuffer/IDirectSoundStream,
 * DirectSoundCreate, WAVEFORMATEX, DSMIXBINS ...) and the xboxkrnl timer/DPC/IRQL
 * + pool primitives (Ke*, Ex*). It links into a title alongside libxapi, libd3d8
 * and libdsound. Built default-__cdecl (the exported IXACT* / XACTEngineCreate
 * carry explicit __stdcall via STDAPI in xactp.h).
 *
 * Same shape as libxmv's site/bridge_xmv.h (a title-side helper that also leans
 * on the public DirectSound surface):
 *  1. Reuse libxapi compile.h for the NT/title build env (NT_INCLUDED so windef
 *     skips zig's MinGW winnt.h; XBOX=1 -- xtl.h is selected via XBOX). This also
 *     pulls xboxkrnl (KTIMER/KDPC/KFLOATING_SAVE, KeGetCurrentIrql/PASSIVE_LEVEL,
 *     ExAllocatePoolWithTag) that the sequencer + memory manager need.
 *  2. Supply the Win32/COM macros our slimmed headers lack, then pull the title
 *     umbrella the engine leans on: <xtl.h> (CRITICAL_SECTION/HANDLE + Win32) and
 *     the PUBLIC <dsound.h> (every DirectSound type xactp.h/xacti.h names).
 *  3. ASSERT/RIP fallbacks + MSVC-CRT shims + retail-build forcing.
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

/* COM calling-convention macros (ntdef.h only defines them under _WIN32). The
   DirectSound COM interfaces (IDirectSoundBuffer/IDirectSoundStream) and the
   XACT COM-style interfaces (IXACTEngine ...) need them. */
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

/* GUID/REFIID for the COM-style dsound decls. */
#include <guiddef.h>

/* HIGH_LEVEL: the top IRQL. xboxkrnl's common.h defines PASSIVE_LEVEL/
   DISPATCH_LEVEL but not HIGH_LEVEL, which debug.h uses to size its (retail-
   unused) g_XactDebugContext[] extern. x86 IRQL max is 31. */
#ifndef HIGH_LEVEL
#define HIGH_LEVEL 31
#endif

/* INITIALIZED_CRITICAL_SECTION: statically initialize an RTL_CRITICAL_SECTION at
   global scope (xactapi.cpp's g_XACTCriticalSection). The XDK supplies this via
   ntrtl.h; replicate the RXDK layout (libxapi/internal/rxdk_ntrtl.h) here so the
   title TU need not pull the full ntrtl_sdk.h. */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG)(LONG_PTR)&(((type *)0)->field))
#endif
#ifndef INITIALIZED_CRITICAL_SECTION
#define INITIALIZED_CRITICAL_SECTION(CriticalSection) \
    RTL_CRITICAL_SECTION CriticalSection = { \
        1, \
        FALSE, \
        (UCHAR)(FIELD_OFFSET(RTL_CRITICAL_SECTION, LockCount) / sizeof(LONG)), \
        FALSE, \
        FALSE, \
        &CriticalSection.Synchronization.Event.WaitListHead, \
        &CriticalSection.Synchronization.Event.WaitListHead, \
        -1, \
        0, \
        NULL \
    }
#endif

/* (2) The title umbrella the engine leans on. xtl.h pulls the Win32/xapi surface
   (CRITICAL_SECTION, HANDLE, EnterCriticalSection ...); <xobjbase.h> supplies the
   Xbox COM interface machinery (DECLARE_INTERFACE/THIS/PURE/LPUNKNOWN) that
   dsound.h's vtables need; <d3dx8math.h> supplies the D3DXVECTOR3 math type its
   DS3D structs use; then the PUBLIC <dsound.h> for every DirectSound type
   xactp.h/xacti.h/wavbndlr.h reference. (Same prelude the dsound-music sample
   builds by hand.) */
#include <xtl.h>
#include <xobjbase.h>
#include <d3dx8math.h>
#include <dsound.h>

/* Engine calls our slimmed headers/import lib don't surface directly:
   - KeInitializeTimer: libkernel exports KeInitializeTimerEx but not the plain
     alias, so provide it inline (KeInitializeTimer(t) == a NotificationTimer),
     which binds to the exported KeInitializeTimerEx -- no missing kernel import.
   - RtlRandom: declared only in the (unincluded) ntrtl_sdk.h, but its object is
     built into libxapi.lib (rtl/random.c), which every title links -- so a plain
     extern decl resolves at title link. */
static __inline VOID KeInitializeTimer(PKTIMER Timer)
{
    KeInitializeTimerEx(Timer, NotificationTimer);
}
EXTERN_C ULONG __stdcall RtlRandom(PULONG Seed);

/* (3) ASSERT / RIP fallbacks. debug.h #undef's + redefines ASSERT/ASSERTMSG per
   the DBG level; these are only the pre-common.h fallbacks (e.g. wavbndlr.h's
   inline SetBufferData ASSERT before common.h's debug.h is seen). */
#ifndef ASSERT
#define ASSERT(x)   ((void)0)
#endif
#ifndef ASSERTMSG
#define ASSERTMSG(m) ((void)0)
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
#include <strings.h>
#ifndef _stricmp
#define _stricmp   strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp  strncasecmp
#endif

/* Force RETAIL: the engine's #if DBG paths pull the XACT debug diagnostics
   (DwDbgPrint and the g_XactDebug globals) we don't build for playback. */
#undef DBG
#undef _DEBUG
#undef DEBUG
