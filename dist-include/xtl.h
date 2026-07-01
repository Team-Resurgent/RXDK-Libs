#pragma once
//
// xtl.h - the RXDK master distribution umbrella (clean-room equivalent of the
// XDK XTL.h). A title includes <xtl.h> alone to get the full public surface:
// the Xbox kernel base types, the Win32/NT subset, COM GUID support, the XAPI
// exports, and the D3D8 / D3DX8 math / DirectSound device APIs.
//
// This header is distribution-only: it lives in dist-include and ships to
// dist\include via build-iso.ps1. It is deliberately NOT in shared\include,
// because that is a source include dir and a public <xtl.h> there would shadow
// libs\libxapi\internal\xtl.h in the in-tree library builds (which are built on
// a different, libxapi/nt type foundation). In-tree code keeps using the
// internal xtl.h; only dist consumers see this one.
//
// Base include order matters:
//   1. xboxkrnl supplies the authoritative base types (DWORD/PVOID/BOOL/HANDLE,
//      the WCHAR=unsigned short universe, ...).
//   2. NT_INCLUDED tells windef.h the NT base is already provided, so it skips
//      its own <winnt.h> and self-supplies the few Win32 bits it still needs
//      (UINT_PTR/INT_PTR, DECLARE_HANDLE).
//   3. windef/winbase add the Win32 API surface; guiddef + xdk_compat add the
//      COM GUID/CRT names the device headers reference; xbox/xkbd add XAPI.
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

//
// Device APIs, in dependency order (the XDK XTL.h pulled the whole DirectX
// surface too). The device public headers assume a prior include set them up
// -- e.g. dsound.h's DS3DBUFFER uses D3DXVECTOR3 from d3dx8math.h -- so pull
// them here so a title gets a working surface from <xtl.h> alone:
//   d3d8      -> _D3DVECTOR and the D3D8 device
//   d3dx8math -> D3DXVECTOR3/matrix math (needs _D3DVECTOR from d3d8)
//   dsound    -> DirectSound (its 3D structs use D3DXVECTOR3)
// XMV, XGraphics and XNet stay opt-in: a title that needs them includes the
// matching header, which now has its prerequisites already in place.
//
#include <d3d8.h>
#include <d3dx8math.h>
#include <dsound.h>
