/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Implements XFactory, the IClassFactory used to create DirectXFile COM
 * objects, tracking server locks and the live instance count.
 */

#include "precomp.h"
#include "xfactory.h"

static long g_cServerLocks = 0;
extern long g_cInstances;

STDMETHODIMP
XFactory::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr;

    ASSERT(ppvObj != NULL);

	{
        *ppvObj = NULL;

        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppvObj = this;
            m_cRef++;
            hr = S_OK;
        } else {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

STDMETHODIMP_(ULONG)
XFactory::AddRef()
{
    m_cRef++;

    return m_cRef;

}

STDMETHODIMP_(ULONG)
XFactory::Release()
{
    ULONG cRef;

    cRef = --m_cRef;

    if (!cRef)
        delete this;

    return cRef;
}

STDMETHODIMP
XFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

#if 0
    if (!VALID_OUT_PTR(ppvObj))
        return E_INVALIDARG;
#endif 0

    HRESULT hr;
    LPDIRECTXFILE pDXFile;

    if (FAILED(hr = DirectXFileCreate(&pDXFile)))
        return hr;

    hr = pDXFile->QueryInterface(riid, ppvObj);

    pDXFile->Release();

    return hr;
}

STDMETHODIMP
XFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cServerLocks);
    else
        InterlockedDecrement(&g_cServerLocks);

    return S_OK;
}

STDAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppvObj)
{
    ASSERT(ppvObj != NULL);

    *ppvObj = NULL;

    if (rclsid != CLSID_CDirectXFile)
        return CLASS_E_CLASSNOTAVAILABLE;

    XFactory *pFactory = new XFactory;

    if (!pFactory)
        return E_OUTOFMEMORY;

    HRESULT hr;

    hr = pFactory->QueryInterface(riid, ppvObj);

    pFactory->Release();

    return hr;
}

STDAPI
DllCanUnloadNow()
{
    HRESULT hr;

    if (!g_cInstances && !g_cServerLocks)
        hr = S_OK;
    else
        hr = S_FALSE;

    return hr;
}
