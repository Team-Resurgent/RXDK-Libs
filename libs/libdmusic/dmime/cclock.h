/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CClock -- the IReferenceClock implementation used by the audio sink.
 */
#ifndef __CCLOCK_H__
#define __CCLOCK_H__

class CAudioSink;

class CClock : public IReferenceClock
{
friend class CAudioSink;
public:

    CClock();

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
private:
    CAudioSink *	m_pParent;      // Pointer to parent structure.

#ifdef XMIX
    DWORD m_dwLastPosition;
    LONGLONG m_llSampleTime;
#endif
};

#endif //__CCLOCK_H__


