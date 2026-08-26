/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CClock - the synthesizer's IReferenceClock implementation. Exposes the
 * master timing source (GetTime, AdviseTime, AdvisePeriodic and friends) that
 * the sequencer and voices use to schedule and render audio.
 */
#ifndef __CCLOCK_H__
#define __CCLOCK_H__

class CDSLink;

class CClock : public IReferenceClock
{
public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /* IReferenceClock methods */
    HRESULT STDMETHODCALLTYPE GetTime( 
        /* [out] */ REFERENCE_TIME __RPC_FAR *pTime);
    
    HRESULT STDMETHODCALLTYPE AdviseTime( 
        /* [in] */ REFERENCE_TIME baseTime,
        /* [in] */ REFERENCE_TIME streamTime,
        /* [in] */ HANDLE hEvent,
        /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE AdvisePeriodic( 
        /* [in] */ REFERENCE_TIME startTime,
        /* [in] */ REFERENCE_TIME periodTime,
        /* [in] */ HANDLE hSemaphore,
        /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE Unadvise( 
        /* [in] */ DWORD dwAdviseCookie);
                CClock();
    void        Init(CDSLink *pDSLink);
    void        Stop();         // Call store current time as offset.
    void        Start();        // Call to reinstate running.
private:
    BOOL        m_fStopped;     // Currently changing configuration.
    CDSLink *	m_pDSLink;      // Pointer to parent DSLink structure.
};

#endif //__CCLOCK_H__


