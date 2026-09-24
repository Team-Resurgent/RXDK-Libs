/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * oledll.h -- declarations for the OLE server DLL registration helpers
 * (RegisterServer / UnregisterServer). Desktop-only; the Xbox build compiles
 * none of it.
 */
#ifndef _OLEDLL_
#define _OLEDLL_
#ifndef _XBOX_

STDAPI
RegisterServer(HMODULE hModule,
               const CLSID &clsid,
               const TCHAR *szFriendlyName,
               const TCHAR *szVerIndProgID,
               const TCHAR *szProgID);

STDAPI
UnregisterServer(const CLSID &clsid,
                 const TCHAR *szFriendlyName,
                 const TCHAR *szVerIndProgID,
                 const TCHAR *szProgID);

BOOL
GetCLSIDRegValue(const CLSID &clsid,
				 const TCHAR *szKey,
				 LPVOID pValue,
				 LPDWORD pcbValue);
				 
HRESULT CLSIDToStr(const CLSID &clsid,
				   TCHAR *szStr,
				   int cbStr);

HRESULT StrToCLSID(TCHAR *szStr,
				   CLSID &clsid,
				   int cbStr);

#endif // _XBOX_
#endif
