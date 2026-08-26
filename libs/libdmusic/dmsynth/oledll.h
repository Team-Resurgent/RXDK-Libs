/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * COM self-registration helpers - RegisterServer / UnregisterServer that write
 * (and remove) the CLSID, ProgID and friendly-name registry entries for the
 * synthesizer's in-process server.
 */
#ifndef _OLEDLL_
#define _OLEDLL_

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

#endif
