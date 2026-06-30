#pragma once
#define RXDK_XMV_BRIDGE_H

/*
 * Force-included before each libxmv translation unit. xmv.lib is the TITLE-SIDE
 * Xbox FMV decoder ported from the May-2020 leak (private/windows/xmv/decoder):
 * a software video codec that decodes XMV packets into a YUY2 D3D surface (via
 * the public d3d8 D3DSurface_* exports). It links into a title alongside libxapi,
 * libd3d8 and libdsound. Built default-__cdecl (the public XMVDecoder_* exports
 * carry explicit __stdcall in xmv.h / xdk_xmv_public_api.c).
 *
 * Same shape as libxgraphics' site/bridge_xgraphics.h (a title-side helper that
 * also leans on the public d3d8 surface):
 *  1. Reuse libxapi compile.h for the NT/title build env (NT_INCLUDED so windef
 *     skips zig's MinGW winnt.h; XBOX=1 -- the decoder selects <xtl.h> via XBOX).
 *  2. Supply the Win32/COM macros our slimmed headers lack, then pull the umbrella
 *     the decoder leans on: <xtl.h> (CreateFile/ReadFile/VirtualAlloc + HANDLE/
 *     OVERLAPPED), <d3d8.h> (D3DSurface/D3DLOCKED_RECT/D3DFMT_YUY2 + the
 *     D3DSurface_* helpers backend.c calls) and <dsound.h> (DSMIXBINS /
 *     IDirectSoundStream referenced by xmv.h's audio entry points).
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
   DirectSound COM interfaces (IDirectSoundStream) and any COM-style decls need them. */
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

/* GUID/REFIID for the COM-style dsound decls. */
#include <guiddef.h>

/* (2) The title umbrella the decoder leans on. xtl.h pulls the Win32/xapi surface
   (file IO + memory); pre-pull <d3d8.h> (the D3DSurface backend.c locks) so the
   per-TU `#include <xmv.h>` resolves. We do NOT pull the full <dsound.h> (it drags
   in the d3dx8 math types + COM interface machinery); xmv.h only names DSMIXBINS /
   IDirectSoundStream as pointer params (audio is NYI in the decoder), so forward-
   declare them with their real dsound.h tags -- the sample includes the full
   <dsound.h> before <xmv.h> for the concrete types. */
#include <xtl.h>
#include <d3d8.h>
#ifndef __IDirectSoundStream_FWD_DEFINED__
#define __IDirectSoundStream_FWD_DEFINED__
typedef struct IDirectSoundStream IDirectSoundStream;
#endif
#ifndef _RXDK_XMV_DSMIXBINS_FWD
#define _RXDK_XMV_DSMIXBINS_FWD
typedef struct _DSMIXBINS DSMIXBINS;
#endif

/* (3) ASSERT / RIP fallbacks. The decoder uses ASSERT() outside of #if DBG
   (backend.c alignment checks, decoder.c packet checks). The real DDK supplies
   it via xdbg.h; in a retail (non-DBG) build it expands to nothing. */
#ifndef ASSERT
#define ASSERT(x)   ((void)0)
#endif
#ifndef ASSERTMSG
#define ASSERTMSG(m, x) ((void)0)
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

/* Force RETAIL: the decoder's #if DBG paths pull XDebugError/DbgPrint diagnostics
   we don't need for playback (see bridge_xgraphics.h). */
#undef DBG
#undef _DEBUG
#undef DEBUG
