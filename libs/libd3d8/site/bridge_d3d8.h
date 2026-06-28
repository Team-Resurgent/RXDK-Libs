#pragma once
#define RXDK_D3D8_BRIDGE_H

/*
 * Force-included (before se/precomp.hpp) for every libd3d8 translation unit.
 * Mirrors libxapi's site/bridge_usb.h: sets up the kernel-runtime include
 * environment that the vendored D3D8 precomp.hpp assumes, without editing the
 * vendor sources. precomp.hpp's later re-includes are guarded no-ops.
 *
 * Two things the vendor precomp.hpp gets wrong for our toolchain:
 *  1. It includes <nturtl.h> (user-mode RTL extensions, which USE
 *     PRTL_DRIVE_LETTER_CURDIR etc.) without <ntrtl.h> first (which DEFINES
 *     them). We pull ntrtl.h before nturtl.h here.
 *  2. The original build got the D3D type/enum defs (D3DCOLOR, D3DFORMAT, ...)
 *     via xtl.h; our libxapi xtl.h shim doesn't pull them, and the se/ internal
 *     headers (hw.h, mp.hpp) use them. We pre-include <d3d8types.h> (types only
 *     -- NOT the public d3d8.h interfaces; the driver implements its own
 *     D3DDevice in device.hpp).
 */

/* D3D8 (the NV2A driver) runs in the kernel runtime, like the USB slices. */
#ifndef NTOS_KERNEL_RUNTIME
#define NTOS_KERNEL_RUNTIME 1
#endif

/*
 * libd3d8 links xboxkrnl.lib directly. Vendor ntos headers mark kernel entry
 * points DECLSPEC_IMPORT/NTKERNELAPI when building as a user-mode driver; clear
 * them so the calls bind to the kernel import library rather than a dll thunk.
 */
#define RXDK_D3D8_LINK 1
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

/* NT include environment, in the order libxapi's own TUs use (ntrtl before
   nturtl). ntos.h in extern "C"; the RTL headers carry their own linkage. */
#ifdef __cplusplus
extern "C" {
#endif
#include <ntos.h>
#ifdef __cplusplus
}
#endif
#include <ntrtl.h>
#include <nturtl.h>

/* Xbox Title Library: brings the Win32 base types (DWORD/BYTE/UINT/HRESULT)
   the D3D headers need. The original build got d3d8types.h via xtl.h too. */
#include <xtl.h>

/* WINAPI/HWND aren't in our slimmed windef.h. d3d8.h uses both (HWND only in the
   vestigial hDeviceWindow field; harmless stub on Xbox). */
/* Use the MSVC calling-convention KEYWORDS (clang accepts them under
   -fms-extensions), not the GCC attribute form: the keyword binds correctly in
   function-pointer typedefs (e.g. state.cpp's render-state fn tables) and in
   out-of-line method defs, where the attribute form mismatches the d3d8.h decls
   and triggers "conflicting types". */
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

/* COM base interface: the driver lets a title register an IUnknown dependency on
   a push-buffer (resource.cpp AddRef/Release it), but we have no unknwn.h. Define
   the standard 3-method COM vtable (QueryInterface/AddRef/Release). */
#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__
struct IUnknown {
    virtual long __stdcall QueryInterface(const struct _GUID &riid, void **ppvObject) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};
#endif

/*
 * The se/ internal headers need the FULL public d3d8.h, not just d3d8types.h:
 * D3DCONST, the D3DFORMAT_*_MASK/SHIFT macros, and D3DSWAPCALLBACK/D3DVBLANKCALLBACK
 * all live there. The original build pulled d3d8.h via xtl.h; our xtl.h shim
 * doesn't. D3DCOMPILE_NOTINLINE makes d3d8.h emit the D3DDevice methods
 * out-of-line (declarations only) so the driver's own se/ definitions provide
 * the bodies -- no conflict with the public inline API.
 */
#ifndef D3DCOMPILE_NOTINLINE
#define D3DCOMPILE_NOTINLINE 1
#endif
#include <d3d8.h>
