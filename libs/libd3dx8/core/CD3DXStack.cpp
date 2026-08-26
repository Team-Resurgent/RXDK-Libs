/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Two small auto-growing stacks used by the D3DX8 effect/shader machinery:
 * CD3DXDwStack pushes and pops DWORDs, and CD3DXSzStack pushes and pops
 * strings (taking a private copy of each on push and freeing them on
 * destruction). Both grow their backing array 16 slots at a time and latch the
 * first failure in an HRESULT that GetLastError returns and clears.
 */

#include "pchcore.h"

//----------------------------------------------------------------------------
// CD3DXDwStack - DWORD stack
//----------------------------------------------------------------------------


CD3DXDwStack::CD3DXDwStack()
{
    m_pdw = NULL;
    m_cdw = 0;
    m_cdwLim = 0;
    m_hr = S_OK;
}

CD3DXDwStack::~CD3DXDwStack()
{
    if(m_pdw)
        delete [] m_pdw;
}

HRESULT
CD3DXDwStack::Push(DWORD dw)
{
    if(m_cdw == m_cdwLim)
    {
        DWORD *pdw;
        
        if(!(pdw = new DWORD[m_cdwLim + 16]))
            return m_hr = E_OUTOFMEMORY;

        if(m_pdw)
        {
            memcpy(pdw, m_pdw, m_cdw * sizeof(DWORD));
            delete [] m_pdw;
        }

        m_pdw = pdw;
        m_cdwLim += 16;
    }

    m_pdw[m_cdw++] = dw;
    return S_OK;
}

HRESULT
CD3DXDwStack::Pop(DWORD *pdw)
{
    if(m_cdw == 0)
    {
        if(pdw)
            *pdw = 0;

        DPF(0, "Stack Empty");
        return m_hr = E_FAIL;
    }

    m_cdw--;

    if(pdw)
        *pdw = m_pdw[m_cdw];

    return S_OK;
}

HRESULT
CD3DXDwStack::GetLastError()
{
    HRESULT hr = m_hr;
    m_hr = S_OK;
    return hr;
}


//----------------------------------------------------------------------------
// CD3DXSzStack - String stack
//----------------------------------------------------------------------------

CD3DXSzStack::CD3DXSzStack()
{
    m_ppsz = NULL;
    m_cpsz = 0;
    m_cpszLim = 0;
    m_hr = S_OK;
}

CD3DXSzStack::~CD3DXSzStack()
{
    if(m_ppsz)
    {
        for(UINT ipsz = 0; ipsz < m_cpsz; ipsz++)
        {
            if(m_ppsz[ipsz])
                delete [] m_ppsz[ipsz];
        }

        delete [] m_ppsz;
    }
}

HRESULT
CD3DXSzStack::Push(char *psz)
{
    if(m_cpsz == m_cpszLim)
    {
        char **ppsz;
        
        if(!(ppsz = new char* [m_cpszLim + 16]))
            return m_hr = E_OUTOFMEMORY;

        if(m_ppsz)
        {
            memcpy(ppsz, m_ppsz, m_cpsz * sizeof(char *));
            delete [] m_ppsz;
        }

        m_ppsz = ppsz;
        m_cpszLim += 16;
    }

    if(psz)
    {
        UINT cch;
        char *pszCopy;

        cch = strlen(psz);

        if(!(pszCopy = new char[cch + 1]))
            return m_hr = E_OUTOFMEMORY;

        memcpy(pszCopy, psz, cch * sizeof(char));
        pszCopy[cch] = 0;

        m_ppsz[m_cpsz++] = pszCopy;
    }
    else
    {
        m_ppsz[m_cpsz++] = NULL;
    }

    return S_OK;
}

HRESULT
CD3DXSzStack::Pop(char **ppsz)
{
    if(m_cpsz == 0)
    {
        if(ppsz)
            *ppsz = NULL;

        DPF(0, "Stack Empty");
        return m_hr = E_FAIL;
    }

    m_cpsz--;

    if(ppsz)
        *ppsz = m_ppsz[m_cpsz];

    return S_OK;
}

HRESULT
CD3DXSzStack::GetLastError()
{
    HRESULT hr = m_hr;
    m_hr = S_OK;
    return hr;
}



