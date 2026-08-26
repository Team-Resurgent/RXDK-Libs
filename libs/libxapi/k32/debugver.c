#include "bridge_k32.h"
/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Debug-only version-query helpers: XDebugGetSystemVersion formats the running
 * kernel version, and XDebugGetXTLVersion formats the title's linked XAPI
 * library version. Both report whether the build is devkit or retail.
 */

#include "basedll.h"
#include "xboxverp.h"
#pragma hdrstop

#ifdef _DEBUG

DWORD
__attribute__((__stdcall__))
XDebugGetSystemVersion(
    PSTR pszVersionString,
    UINT cchVersionString
    )
{
    RIP_ON_NOT_TRUE("XDebugGetSystemVersion()", (pszVersionString != NULL));

    _snprintf(pszVersionString,
              cchVersionString,
              "%d.%02d.%d.%02d",
              XboxKrnlVersion->Major,
              XboxKrnlVersion->Minor,
              XboxKrnlVersion->Build,
              (XboxKrnlVersion->Qfe & 0x7FFF));

    return (XboxKrnlVersion->Qfe & 0x8000) ? XVER_DEVKIT : XVER_RETAIL;
}

DWORD
__attribute__((__stdcall__))
XDebugGetXTLVersion(
    PSTR pszVersionString,
    UINT cchVersionString
    )
{
    RIP_ON_NOT_TRUE("XDebugGetXTLVersion()", (pszVersionString != NULL));

    _snprintf(pszVersionString,
              cchVersionString,
              "%d.%02d.%d.%02d",
              XeImageHeader()->XapiLibraryVersion->MajorVersion,
              XeImageHeader()->XapiLibraryVersion->MinorVersion,
              XeImageHeader()->XapiLibraryVersion->BuildVersion,
              XeImageHeader()->XapiLibraryVersion->QFEVersion);

    return XeImageHeader()->XapiLibraryVersion->DebugBuild ? XVER_DEVKIT : XVER_RETAIL;
}

#endif // _DEBUG
