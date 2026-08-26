/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CPhoneyDSound -- a lightweight IDirectSound8 stand-in that DirectMusic uses on
 * Xbox to drive the audio sink without a full DirectSound device.
 */

#include "pchime.h"
/*#ifdef XBOX
#include <xtl.h>
#else // XBOX
#include <windows.h>
#endif // XBOX

#include <mmsystem.h>
#include <dsoundp.h>
#include "dmsegobj.h"
#include "dmgraph.h"
#include "dmperf.h"
#include "dmusici.h"
#include "..\shared\Validate.h"
#include "audpath.h"
#include "debug.h"
#include "..\shared\dmusiccp.h"
#include "..\shared\xcreate.h"
//#include "dslink.h"
#include "audiosink.h"
#include "phoneyds.h"*/


CPhoneyDSound::CPhoneyDSound()
{
    m_cRef = 0;
}

CPhoneyDSound::~CPhoneyDSound()
{
}

/*STDMETHODIMP CPhoneyDSound::QueryInterface(const IID &iid, void **ppv)
{
    if(iid == IID_IUnknown || iid == IID_IDirectSound || iid == IID_IDirectSound8)
    {
        *ppv = static_cast<IDirectSound8*>(this);
    } 
    else if(iid == IID_IDirectSoundPrivate) 
    {
        *ppv = static_cast<IDirectSoundPrivate*>(this);
    } 
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    
    return S_OK;
}*/

STDMETHODIMP_(ULONG) CPhoneyDSound::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CPhoneyDSound::Release()
{
    if(!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CPhoneyDSound::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) 
{ 
    CBuffer *pBuffer = new CBuffer(NULL,NULL,NULL,0,GUID_NULL);
    if (pBuffer)
    {
        pBuffer->AddRef();
        (*ppDSBuffer) = (LPDIRECTSOUNDBUFFER) pBuffer;
        return S_OK;
    }
    return E_OUTOFMEMORY; 
}

STDMETHODIMP CPhoneyDSound::GetCaps(LPDSCAPS pDSCaps) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::SetCooperativeLevel(HWND hwnd, DWORD dwLevel) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::Compact() 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::GetSpeakerConfig(LPDWORD pdwSpeakerConfig) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::SetSpeakerConfig(DWORD dwSpeakerConfig) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::Initialize(LPCGUID pcGuidDevice) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPhoneyDSound::VerifyCertification(LPDWORD pdwCertified) 
{ 
    return E_NOTIMPL; 
}

// IDirectSoundPrivate methods
STDMETHODIMP CPhoneyDSound::AllocSink(LPWAVEFORMATEX pwfxFormat, LPDIRECTSOUNDCONNECT *ppSinkConnect) 
{ 
    CAudioSink *pSink = new CAudioSink(pwfxFormat);
    if (pSink)
    {
        return pSink->QueryInterface(IID_IDirectSoundConnect,(void **)ppSinkConnect);
    }
    return E_OUTOFMEMORY;
}

