/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Declaration of CD3DXFile, the whole-file reader. See CD3DXFile.cpp.
 */

#ifndef __CD3DXFile_H__
#define __CD3DXFile_H__


///////////////////////////////////////////////////////////////////////////
// CD3DXFile //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

class CD3DXFile
{
public:
    LPCVOID     m_pvData;
    DWORD       m_cbData;

public:
    CD3DXFile();
    ~CD3DXFile();

    HRESULT Open(LPCVOID pFile, BOOL bUnicode);
    HRESULT Close();
};



#endif
