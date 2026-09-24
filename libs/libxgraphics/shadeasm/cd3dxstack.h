/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Declarations of the growable stack containers used by the shader assembler
 * (XAsmCD3DXDwStack for DWORDs, XAsmCD3DXSzStack for strings, and related
 * templates). Implementations live in cd3dxstack.cpp.
 */

namespace XGRAPHICS {

//----------------------------------------------------------------------------
// XAsmCD3DXDwStack
//----------------------------------------------------------------------------


class XAsmCD3DXDwStack
{
    DWORD *m_pdw;

    UINT m_cdw;
    UINT m_cdwLim;

    HRESULT m_hr;

public:
    XAsmCD3DXDwStack();
    ~XAsmCD3DXDwStack();

    HRESULT Push(DWORD dw);
    HRESULT Pop (DWORD *pdw);

    HRESULT GetLastError();
};


//----------------------------------------------------------------------------
// XAsmCD3DXSzStack
//----------------------------------------------------------------------------

class XAsmCD3DXSzStack
{
    char **m_ppsz;
    UINT m_cpsz;
    UINT m_cpszLim;
    HRESULT m_hr;

public:
    XAsmCD3DXSzStack();
    ~XAsmCD3DXSzStack();

    HRESULT Push(char *psz);
    HRESULT Pop (char **ppsz);

    HRESULT GetLastError();
};

} // namespace XGRAPHICS