/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled-header include file - the frequently used, rarely changed system
 * and ATL headers plus the project-wide build switches (STRICT, _WIN32_WINNT,
 * apartment-threaded ATL) shared across the module.
 */

#if !defined(AFX_STDAFX_H__58C2B4C5_46E7_11D1_89AC_00A0C9054129__INCLUDED_)
#define AFX_STDAFX_H__58C2B4C5_46E7_11D1_89AC_00A0C9054129__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__58C2B4C5_46E7_11D1_89AC_00A0C9054129__INCLUDED)
