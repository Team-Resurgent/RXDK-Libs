/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declarations of the D3DX8 helper stacks CD3DXDwStack (DWORD stack) and
 * CD3DXSzStack (string stack). See CD3DXStack.cpp for the behaviour.
 */



//----------------------------------------------------------------------------
// CD3DXDwStack
//----------------------------------------------------------------------------


class CD3DXDwStack
{
    DWORD *m_pdw;

    UINT m_cdw;
    UINT m_cdwLim;

    HRESULT m_hr;

public:
    CD3DXDwStack();
    ~CD3DXDwStack();

    HRESULT Push(DWORD dw);
    HRESULT Pop (DWORD *pdw);

    HRESULT GetLastError();
};


//----------------------------------------------------------------------------
// CD3DXSzStack
//----------------------------------------------------------------------------

class CD3DXSzStack
{
    char **m_ppsz;
    UINT m_cpsz;
    UINT m_cpszLim;
    HRESULT m_hr;

public:
    CD3DXSzStack();
    ~CD3DXSzStack();

    HRESULT Push(char *psz);
    HRESULT Pop (char **ppsz);

    HRESULT GetLastError();
};
