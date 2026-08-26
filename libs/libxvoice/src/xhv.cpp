/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// The XHV high-level voice-chat engine (xhv.h / xvoice.lib): an implementation
// of the public C surface.
//
// What is REAL here: engine lifetime/refcounting, processing-mode bookkeeping,
// the title callback interface, local/remote talker registration and
// enumeration, voice masks, mix-bin/priority/stream-count state. A title can
// drive the whole XHV API and observe consistent state.
//
// What is intentionally INERT: everything that needs hardware or codecs RXDK
// does not provide -- the voice communicator USB audio class driver (so no
// communicator can ever be inserted; GetLocalTalkerStatus faithfully reports
// REMOVED, exactly like retail with no headset plugged in), the SC03 voice
// codec (SubmitIncomingVoicePacket accepts and discards, no audio is rendered,
// IsTalking is FALSE) and the WMAVoice voicemail codec + speech recognition
// (VoiceMail*/SR entry points fail cleanly). DoWork is a no-op that succeeds,
// so title main loops run unchanged.
//
// This is a CAPABILITY BOUNDARY, not unfinished work: the communicator USB
// audio class driver and the SC03/WMAVoice codecs ship only as binaries in the
// retail libs. Audible voice would require writing a USB audio driver and a
// codec from scratch (or licensing one), which is out of scope for an SDK
// reconstruction.

#include <xonline.h> // XUID (xhv.h insists it is included first)
#include <xhv.h>

#include <stdlib.h> // malloc/free (picolibc)
#include <string.h> // memset/memcmp

// ------------------------------------------------------------------ modes ---
// The five processing-mode handles are opaque PVOID constants; each points at
// its own static token so titles get five distinct, comparable values.
static const BYTE s_modeTokens[5] = { 0, 1, 2, 3, 4 };

extern "C" {
const XHV_PROCESSING_MODE _xhv_inactive_mode  = (XHV_PROCESSING_MODE)&s_modeTokens[0];
const XHV_PROCESSING_MODE _xhv_loopback_mode  = (XHV_PROCESSING_MODE)&s_modeTokens[1];
const XHV_PROCESSING_MODE _xhv_voicechat_mode = (XHV_PROCESSING_MODE)&s_modeTokens[2];
const XHV_PROCESSING_MODE _xhv_sr_mode        = (XHV_PROCESSING_MODE)&s_modeTokens[3];
const XHV_PROCESSING_MODE _xhv_voicemail_mode = (XHV_PROCESSING_MODE)&s_modeTokens[4];
}

static int ModeIndex(XHV_PROCESSING_MODE mode)
{
    const BYTE *p = (const BYTE *)mode;
    if (p >= s_modeTokens && p < s_modeTokens + 5)
        return (int)(p - s_modeTokens);
    return -1;
}

// ----------------------------------------------------------------- engine ---

struct LOCAL_TALKER
{
    BOOL                fRegistered;
    XHV_PROCESSING_MODE mode;
    XHV_VOICE_MASK      mask;
};

struct REMOTE_TALKER
{
    BOOL fUsed;
    XUID xuid;
};

// The public XHVEngine is opaque (xhv.h only forward-declares it and routes
// every method through the exported C functions), so the layout is ours.
struct RXDK_XHV_ENGINE
{
    LONG                lRefCount;
    DWORD               dwEnabledModes; // bitmask by mode index
    ITitleXHV          *pCallback;
    DWORD               dwMaxRemoteTalkers;
    DWORD               dwMaxLocalTalkers;
    DWORD               dwMaxPlaybackStreams;
    PVOID               pvSRBank;
    LOCAL_TALKER        local[XHV_MAX_LOCAL_TALKERS];
    REMOTE_TALKER       remote[XHV_MAX_REMOTE_TALKERS];
};

static RXDK_XHV_ENGINE *Eng(XHVEngine *pThis)
{
    return (RXDK_XHV_ENGINE *)pThis;
}

static BOOL SameXuid(const XUID &a, const XUID &b)
{
    return a.qwUserID == b.qwUserID && a.dwUserFlags == b.dwUserFlags;
}

static REMOTE_TALKER *FindRemote(RXDK_XHV_ENGINE *e, XUID xuid)
{
    for (DWORD i = 0; i < XHV_MAX_REMOTE_TALKERS; i++)
        if (e->remote[i].fUsed && SameXuid(e->remote[i].xuid, xuid))
            return &e->remote[i];
    return NULL;
}

extern "C" {

XBOXAPI HRESULT WINAPI XHVEngineCreate(PXHV_RUNTIME_PARAMS pParams, PXHVENGINE *ppEngine)
{
    if (!ppEngine)
        return E_POINTER;
    *ppEngine = NULL;
    if (!pParams)
        return E_INVALIDARG;
    if (pParams->dwMaxRemoteTalkers > XHV_MAX_REMOTE_TALKERS ||
        pParams->dwMaxLocalTalkers > XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;

    RXDK_XHV_ENGINE *e = (RXDK_XHV_ENGINE *)malloc(sizeof(RXDK_XHV_ENGINE));
    if (!e)
        return E_OUTOFMEMORY;
    memset(e, 0, sizeof(*e));
    e->lRefCount            = 1;
    e->pCallback            = NULL;
    e->dwMaxRemoteTalkers   = pParams->dwMaxRemoteTalkers ? pParams->dwMaxRemoteTalkers
                                                          : XHV_MAX_REMOTE_TALKERS;
    e->dwMaxLocalTalkers    = pParams->dwMaxLocalTalkers ? pParams->dwMaxLocalTalkers
                                                         : XHV_MAX_LOCAL_TALKERS;
    e->dwMaxPlaybackStreams = XHV_MAX_PLAYBACK_STREAMS;
    for (int i = 0; i < XHV_MAX_LOCAL_TALKERS; i++)
        e->local[i].mode = _xhv_inactive_mode;

    *ppEngine = (XHVEngine *)e;
    return S_OK;
}

ULONG WINAPI XHVEngine_AddRef(XHVEngine *pThis)
{
    return (ULONG)(++Eng(pThis)->lRefCount);
}

ULONG WINAPI XHVEngine_Release(XHVEngine *pThis)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    LONG cRef = --e->lRefCount;
    if (cRef <= 0) {
        free(e);
        return 0;
    }
    return (ULONG)cRef;
}

HRESULT WINAPI XHVEngine_EnableProcessingMode(XHVEngine *pThis, XHV_PROCESSING_MODE processingMode)
{
    int idx = ModeIndex(processingMode);
    if (idx < 0)
        return E_INVALIDARG;
    Eng(pThis)->dwEnabledModes |= (1u << idx);
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetCallbackInterface(XHVEngine *pThis, ITitleXHV *pITitleXHV)
{
    Eng(pThis)->pCallback = pITitleXHV;
    return S_OK;
}

HRESULT WINAPI XHVEngine_GetCallbackInterface(XHVEngine *pThis, ITitleXHV **ppITitleXHV)
{
    if (!ppITitleXHV)
        return E_POINTER;
    *ppITitleXHV = Eng(pThis)->pCallback;
    return S_OK;
}

// Per-frame pump. The retail engine encodes/decodes queued voice here and
// fires the ITitleXHV callbacks; with no communicator driver and no codec
// there is never work queued, so succeeding without side effects is exactly
// the no-headset behavior.
HRESULT WINAPI XHVEngine_DoWork(XHVEngine *pThis)
{
    (void)pThis;
    return S_OK;
}

HRESULT WINAPI XHVEngine_RegisterLocalTalker(XHVEngine *pThis, DWORD dwLocalPort)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;
    e->local[dwLocalPort].fRegistered = TRUE;
    e->local[dwLocalPort].mode        = _xhv_inactive_mode;
    return S_OK;
}

HRESULT WINAPI XHVEngine_UnregisterLocalTalker(XHVEngine *pThis, DWORD dwLocalPort)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;
    e->local[dwLocalPort].fRegistered = FALSE;
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetProcessingMode(XHVEngine *pThis, DWORD dwLocalPort,
                                           XHV_PROCESSING_MODE processingMode)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS || ModeIndex(processingMode) < 0)
        return E_INVALIDARG;
    if (!e->local[dwLocalPort].fRegistered)
        return E_FAIL;
    e->local[dwLocalPort].mode = processingMode;
    return S_OK;
}

HRESULT WINAPI XHVEngine_GetProcessingMode(XHVEngine *pThis, DWORD dwLocalPort,
                                           XHV_PROCESSING_MODE *pProcessingMode)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (!pProcessingMode)
        return E_POINTER;
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;
    *pProcessingMode = e->local[dwLocalPort].mode;
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetVoiceMask(XHVEngine *pThis, DWORD dwLocalPort,
                                      const XHV_VOICE_MASK *pVoiceMask)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (!pVoiceMask)
        return E_POINTER;
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;
    e->local[dwLocalPort].mask = *pVoiceMask;
    return S_OK;
}

HRESULT WINAPI XHVEngine_GetLocalTalkerStatus(XHVEngine *pThis, DWORD dwLocalPort,
                                              XHV_LOCAL_TALKER_STATUS *pLocalTalkerStatus)
{
    if (!pLocalTalkerStatus)
        return E_POINTER;
    if (dwLocalPort >= XHV_MAX_LOCAL_TALKERS)
        return E_INVALIDARG;
    (void)pThis;
    // No communicator USB driver exists in RXDK, so no headset can ever be
    // inserted -- the same status retail reports with an empty port.
    pLocalTalkerStatus->communicatorStatus = XHV_VOICE_COMMUNICATOR_STATUS_REMOVED;
    pLocalTalkerStatus->bIsTalking         = FALSE;
    return S_OK;
}

HRESULT WINAPI XHVEngine_RegisterRemoteTalker(XHVEngine *pThis, XUID xuidRemoteTalker)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (FindRemote(e, xuidRemoteTalker))
        return S_OK; // already registered
    DWORD cUsed = 0;
    REMOTE_TALKER *pFree = NULL;
    for (DWORD i = 0; i < XHV_MAX_REMOTE_TALKERS; i++) {
        if (e->remote[i].fUsed)
            cUsed++;
        else if (!pFree)
            pFree = &e->remote[i];
    }
    if (!pFree || cUsed >= e->dwMaxRemoteTalkers)
        return E_OUTOFMEMORY;
    pFree->fUsed = TRUE;
    pFree->xuid  = xuidRemoteTalker;
    return S_OK;
}

HRESULT WINAPI XHVEngine_UnregisterRemoteTalker(XHVEngine *pThis, XUID xuidRemoteTalker)
{
    REMOTE_TALKER *r = FindRemote(Eng(pThis), xuidRemoteTalker);
    if (!r)
        return E_INVALIDARG;
    r->fUsed = FALSE;
    return S_OK;
}

HRESULT WINAPI XHVEngine_GetRemoteTalkers(XHVEngine *pThis, DWORD *pdwRemoteTalkersCount,
                                          XUID *pxuidRemoteTalkers)
{
    RXDK_XHV_ENGINE *e = Eng(pThis);
    if (!pdwRemoteTalkersCount)
        return E_POINTER;
    DWORD c = 0;
    for (DWORD i = 0; i < XHV_MAX_REMOTE_TALKERS; i++) {
        if (e->remote[i].fUsed) {
            if (pxuidRemoteTalkers)
                pxuidRemoteTalkers[c] = e->remote[i].xuid;
            c++;
        }
    }
    *pdwRemoteTalkersCount = c;
    return S_OK;
}

// Incoming network voice data. Accepted (the talker must be registered, as on
// retail) but discarded: decoding it needs the SC03 codec the leak lacks.
HRESULT WINAPI XHVEngine_SubmitIncomingVoicePacket(XHVEngine *pThis, XUID xuidRemoteTalker,
                                                   VOID *pvData, DWORD dwSize)
{
    (void)pvData;
    (void)dwSize;
    if (!FindRemote(Eng(pThis), xuidRemoteTalker))
        return E_INVALIDARG;
    return S_OK;
}

BOOL WINAPI XHVEngine_IsTalking(XHVEngine *pThis, XUID xuidRemoteTalker)
{
    (void)pThis;
    (void)xuidRemoteTalker;
    return FALSE; // no decode path, nobody is ever mid-utterance
}

HRESULT WINAPI XHVEngine_SetMixBinMapping(XHVEngine *pThis, XUID xuidRemoteTalker,
                                          DWORD dwLocalPort, const DSMIXBINS *pMixBins)
{
    (void)dwLocalPort;
    (void)pMixBins;
    if (!FindRemote(Eng(pThis), xuidRemoteTalker))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetMixBinVolumes(XHVEngine *pThis, XUID xuidRemoteTalker,
                                          const DSMIXBINS *pMixBins)
{
    (void)pMixBins;
    if (!FindRemote(Eng(pThis), xuidRemoteTalker))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetPlaybackPriority(XHVEngine *pThis, XUID xuidRemoteTalker,
                                             DWORD dwLocalPort, XHV_PLAYBACK_PRIORITY playbackPriority)
{
    (void)dwLocalPort;
    (void)playbackPriority;
    if (!FindRemote(Eng(pThis), xuidRemoteTalker))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT WINAPI XHVEngine_SetMaxPlaybackStreamsCount(XHVEngine *pThis, DWORD dwStreamsCount)
{
    if (dwStreamsCount > XHV_MAX_PLAYBACK_STREAMS)
        return E_INVALIDARG;
    Eng(pThis)->dwMaxPlaybackStreams = dwStreamsCount;
    return S_OK;
}

HRESULT WINAPI XHVEngine_RegisterSpeechRecognitionBank(XHVEngine *pThis, VOID *pvSRBank)
{
    Eng(pThis)->pvSRBank = pvSRBank;
    return S_OK;
}

// Speech recognition needs the SR runtime inside the retail xvoice.lib (no
// source anywhere in the leak) -- fail cleanly so titles degrade.
HRESULT WINAPI XHVEngine_SelectVocabulary(XHVEngine *pThis, DWORD dwLocalPort,
                                          DWORD dwVocabsCount,
                                          const XHV_SR_VOCAB_SELECTION *pVocabSelections)
{
    (void)pThis;
    (void)dwLocalPort;
    (void)dwVocabsCount;
    (void)pVocabSelections;
    return E_FAIL;
}

// Voicemail records/plays through the WMAVoice codec (absent from the leak);
// with no communicator there is also nothing to record. Fail cleanly.
HRESULT WINAPI XHVEngine_VoiceMailRecord(XHVEngine *pThis, DWORD dwLocalPort, DWORD dwMaxTimeMs,
                                         DWORD dwOutputBufferSize, BYTE *pbOutputBuffer)
{
    (void)pThis;
    (void)dwLocalPort;
    (void)dwMaxTimeMs;
    (void)dwOutputBufferSize;
    (void)pbOutputBuffer;
    return E_FAIL;
}

HRESULT WINAPI XHVEngine_VoiceMailPlay(XHVEngine *pThis, DWORD dwLocalPort, DWORD dwInputBufferSize,
                                       const BYTE *pbInputBuffer, BOOL bOutputToSpeakers)
{
    (void)pThis;
    (void)dwLocalPort;
    (void)dwInputBufferSize;
    (void)pbInputBuffer;
    (void)bOutputToSpeakers;
    return E_FAIL;
}

HRESULT WINAPI XHVEngine_VoiceMailStop(XHVEngine *pThis, DWORD dwLocalPort)
{
    (void)pThis;
    (void)dwLocalPort;
    return S_OK; // nothing is ever recording/playing; stopping trivially succeeds
}

} // extern "C"
