/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_XVOICE_BRIDGE_H

/*
 * Force-included before each libxvoice translation unit. xvoice.lib is the
 * voice library: the XHV high-level voice-chat engine (xhv.h) plus the
 * low-level voice XMO/codec API (xvoice.h) and the XDEVICE_TYPE_VOICE_* device
 * tables. It implements the public C surface: engine/talker bookkeeping is real;
 * anything that needs the voice communicator USB audio driver or the SC03/WMAVoice
 * codecs (which RXDK does not provide) reports the same failure the retail lib
 * produces when no communicator is inserted. See libs/libxvoice/sources.zig.
 *
 * Same shape as libxact's site/bridge_xact.h (a title-side lib built on the
 * public header surface):
 *  1. Reuse libxapi compile.h for the NT/title build env (NT_INCLUDED so windef
 *     skips zig's MinGW winnt.h; XBOX=1 -- xtl.h is selected via XBOX).
 *  2. Supply the Win32/COM macros our slimmed headers lack, then pull the title
 *     umbrella: <xtl.h>, <xobjbase.h> (DECLARE_INTERFACE/THIS_/PURE for the
 *     ITitleXHV + XMO vtables), <d3dx8math.h> + the PUBLIC <dsound.h>
 *     (DSMIXBINS/DSEFFECTIMAGEDESC/XMediaObject that xhv.h/xvoice.h name).
 *     The TUs themselves include <xonline.h> + <xhv.h> or <xvoice.h> (those two
 *     are mutually exclusive per header design, so they cannot live here).
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

/* COM calling-convention macros (ntdef.h only defines them under _WIN32). */
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

/* (2) The title umbrella the voice headers lean on. */
#include <xtl.h>
#include <xobjbase.h>
#include <d3dx8math.h>
#include <dsound.h>
