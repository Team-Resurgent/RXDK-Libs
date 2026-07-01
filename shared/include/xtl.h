#pragma once
//
// xtl.h - the RXDK master distribution umbrella (clean-room equivalent of the
// XDK XTL.h). A title includes <xtl.h> alone to get the public base surface:
// the Xbox kernel base types, the Win32/NT subset, COM GUID support, and the
// XAPI exports.
//
// This header lives at the include root and belongs to the SDK distribution
// itself, not to any single library: the SDK's own component headers
// (<dsound.h>, <d3d8.h>, ...) include <xtl.h> for this base, then add their
// device API on top. Titles include <xtl.h> plus whichever device headers they
// use.
//
// Include order matters:
//   1. xboxkrnl supplies the authoritative base types (DWORD/PVOID/BOOL/HANDLE,
//      the WCHAR=unsigned short universe, ...).
//   2. NT_INCLUDED tells windef.h the NT base is already provided, so it skips
//      its own <winnt.h> and self-supplies the few Win32 bits it still needs
//      (UINT_PTR/INT_PTR, DECLARE_HANDLE).
//   3. windef/winbase add the Win32 API surface; guiddef adds the COM GUID
//      types/macros (GUID/REFIID/DECLSPEC_SELECTANY) the D3D8/DirectSound
//      interfaces reference; xbox/xkbd add the XAPI exports.
//
#include <xboxkrnl/xboxkrnl.h>

#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

#include <stdarg.h>   /* va_list, used by winbase.h (matches XDK XTL.h) */
#include <windef.h>
#include <winbase.h>
#include <guiddef.h>     /* GUID/REFGUID/REFIID/DECLSPEC_SELECTANY for COM interfaces */
#include <xdk_compat.h>  /* Win32/COM names the device public headers need (see file) */
#include <xbox.h>
#include <xkbd.h>
