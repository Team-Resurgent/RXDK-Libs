/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of CPhoneyDSound, the IDirectSound8 stand-in that drives the
 * DirectMusic audio sink.
 */

#ifndef __PHONEY_DS__
#define __PHONEY_DS__

#include <math.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include "dmusicc.h"
#include "dmusics.h"
#include "cclock.h"
#include "PLClock.h"
#include "audiosink.h"


class CPhoneyDSound : public IDirectSound8, public IDirectSoundPrivate 
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectSound methods
    STDMETHODIMP CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) ;
    STDMETHODIMP GetCaps(LPDSCAPS pDSCaps) ;
    STDMETHODIMP DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) ;
    STDMETHODIMP SetCooperativeLevel(HWND hwnd, DWORD dwLevel) ;
    STDMETHODIMP Compact() ;
    STDMETHODIMP GetSpeakerConfig(LPDWORD pdwSpeakerConfig) ;
    STDMETHODIMP SetSpeakerConfig(DWORD dwSpeakerConfig) ;
    STDMETHODIMP Initialize(LPCGUID pcGuidDevice) ;
    STDMETHODIMP VerifyCertification(LPDWORD pdwCertified) ;

    // IDirectSoundPrivate methods
    STDMETHODIMP AllocSink(LPWAVEFORMATEX pwfxFormat, LPDIRECTSOUNDCONNECT *ppSinkConnect) ;

    CPhoneyDSound();
    ~CPhoneyDSound();
private:
    long        m_cRef;
};
#endif // __PHONEY_DS__
