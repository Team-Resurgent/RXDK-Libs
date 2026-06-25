#pragma once
//
// xt.h - the libxapi distribution umbrella (clean-room equivalent of the XDK
// XTL.h). A consumer includes <xt.h> alone to get the full public surface.
//
// Include order matters:
//   1. xboxkrnl supplies the authoritative base types (DWORD/PVOID/BOOL/HANDLE,
//      the WCHAR=unsigned short universe, ...).
//   2. NT_INCLUDED tells windef.h the NT base is already provided, so it skips
//      its own <winnt.h> and self-supplies the few Win32 bits it still needs
//      (UINT_PTR/INT_PTR, DECLARE_HANDLE).
//   3. windef/winbase add the Win32 API surface; xbox/xkbd add the XAPI exports.
//
#include <xboxkrnl/xboxkrnl.h>

#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

#include <stdarg.h>   /* va_list, used by winbase.h (matches XDK XTL.h) */
#include <windef.h>
#include <winbase.h>
#include <xbox.h>
#include <xkbd.h>
