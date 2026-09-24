/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Miscellaneous shared declarations used across more than one of the
 * DirectMusic modules - small helper prototypes and optional debug-memory
 * hooks.
 */
#ifndef __MISC_H__
#define __MISC_H__

//LPVOID MemGlobalAllocPtr(UINT uFlags,DWORD dwBytes);
//BOOL MemGlobalFreePtr(LPVOID p);

// memory functions
//HRESULT MemStart();
//void MemEnd();

/*#ifdef _DEBUGMEM
#ifndef new
void* operator new( size_t cb, LPCTSTR pszFileName, WORD wLine );
#define new new( __FILE__, (WORD)__LINE__ )
#endif
#endif*/

#ifdef DBG
#define RELEASE( obj ) ( (obj)->Release(), *((char**)&(obj)) = (char*)0x0bad0bad )
#else
#define RELEASE( obj ) (obj)->Release()
#endif

#ifndef XBOX

BOOL GetRegValueDword(
    LPCTSTR szRegPath,
    LPCTSTR szValueName,
    LPDWORD pdwValue);

#endif // ! XBOX

ULONG GetTheCurrentTime();

#endif // __MISC_H__