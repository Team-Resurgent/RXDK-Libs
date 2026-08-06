// RXDK 5849 uplift: exports the XACT engine APIs that the imported 5849 samples reference but that
// are shaped/named differently in (or absent from) the Jan-2002 source leak.
//
//   * PrepareEx -- 5849 turned SoundBank Play into an inline over PrepareEx. The leak has no
//     "prepare a cue without playing" path, so PrepareEx forwards to the leak's Play.
//
//   * Per-component 3D listener setters -- FUNCTIONAL. 5849 split the leak's combined
//     SetListenerParameters into Position/Velocity/Orientation. We keep a persistent DS3DLISTENER on
//     the engine, update the relevant component, and delegate to the (real) SetListenerParameters.
//
//   * Runtime variables -- FUNCTIONAL storage. SetVariable/GetVariable round-trip through a table on
//     the engine. (What is NOT implemented is RPC modulation -- variables driving sound parameters --
//     which needs an RPC/curve engine the leak does not have.)
//
//   * SetParameterControl -- the RPC/parameter-control engine is entirely new in 5849 and absent
//     from the leak; this remains a no-op stub (returns S_OK) so titles link and boot.

#include "xacti.h"

using namespace XACT;

// ---- CEngine 5849 methods --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CEngine::SetListenerPosition(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    m_ds3dListener.vPosition.x = x;
    m_ds3dListener.vPosition.y = y;
    m_ds3dListener.vPosition.z = z;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

HRESULT STDMETHODCALLTYPE CEngine::SetListenerVelocity(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    m_ds3dListener.vVelocity.x = x;
    m_ds3dListener.vVelocity.y = y;
    m_ds3dListener.vVelocity.z = z;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

HRESULT STDMETHODCALLTYPE CEngine::SetListenerOrientation(FLOAT xFront, FLOAT yFront, FLOAT zFront,
                                                          FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply)
{
    m_ds3dListener.vOrientFront.x = xFront;
    m_ds3dListener.vOrientFront.y = yFront;
    m_ds3dListener.vOrientFront.z = zFront;
    m_ds3dListener.vOrientTop.x = xTop;
    m_ds3dListener.vOrientTop.y = yTop;
    m_ds3dListener.vOrientTop.z = zTop;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

//
// RXDK 5849 uplift: the three remaining CEngine APIs, recovered from
// xacteng.lib (docs/5849-xact-api-recovery.md).
//
// EnableHeadphones and SetI3dl2Listener are forwarders in retail too --
// EnableHeadphones onto IDirectSound_EnableHeadphones, SetI3dl2Listener onto
// IDirectSound_SetI3DL2Listener (retail routes the latter through a static
// CSoundSource helper that also caches the parameters for its own 3D math;
// this port's 3D math lives in libdsound, so the forward is the whole job --
// and SetListenerParameters is already that forward).
//

HRESULT STDMETHODCALLTYPE CEngine::EnableHeadphones(BOOL fEnabled)
{
    ENTER_EXTERNAL_METHOD();
    return m_pDirectSound->EnableHeadphones(fEnabled);
}

HRESULT STDMETHODCALLTYPE CEngine::SetI3dl2Listener(LPCDSI3DL2LISTENER pds3dl, DWORD dwApply)
{
    return SetListenerParameters(NULL, pds3dl, dwApply);
}

//
// Retail: IDirectSound_GetCaps into DSoundCaps, IDirectSound_GetOutputLevels
// into OutputLevels (without resetting the peaks), bail on the first failure,
// then the engine's availability counters and the running allocation total.
//

static BYTE CountListEntries(const LIST_ENTRY *pList)
{
    DWORD dwCount = 0;

    for (const LIST_ENTRY *pEntry = pList->Flink; pEntry != pList; pEntry = pEntry->Flink) {
        if (++dwCount == 255) {
            break;
        }
    }

    return (BYTE)dwCount;
}

HRESULT STDMETHODCALLTYPE CEngine::GetRealtimeData(PXACT_REALTIME_AUDIO_DATA pData)
{
    HRESULT hr;

    ENTER_EXTERNAL_METHOD();

    if (!pData) {
        return E_INVALIDARG;
    }

    hr = m_pDirectSound->GetCaps(&pData->DSoundCaps);

    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pDirectSound->GetOutputLevels(&pData->OutputLevels, FALSE);

    if (FAILED(hr)) {
        return hr;
    }

    pData->dwXactMemoryUsage = (DWORD)g_lXactMemoryUsage;
    pData->bXactAvailable2DBuffers = CountListEntries(&m_lstAvailable2DBuffers);
    pData->bXactAvailable2DStreams = CountListEntries(&m_lstAvailableStreams);
    pData->bXactAvailable3DBuffers = CountListEntries(&m_lstAvailable3DBuffers);
    pData->bReserved = 0;

    return hr;
}

HRESULT STDMETHODCALLTYPE CEngine::SetVariable(DWORD dwVariable, WORD wValue)
{
    if (dwVariable >= (sizeof(m_aVariables) / sizeof(m_aVariables[0])))
        return E_INVALIDARG;
    m_aVariables[dwVariable] = wValue;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CEngine::GetVariable(DWORD dwVariable, PWORD pwValue)
{
    if (!pwValue)
        return E_POINTER;
    if (dwVariable >= (sizeof(m_aVariables) / sizeof(m_aVariables[0])))
        return E_INVALIDARG;
    *pwValue = m_aVariables[dwVariable];
    return S_OK;
}

// ---- exported C entry points -----------------------------------------------------------------

STDAPI IXACTSoundBank_PrepareEx(PXACTSOUNDBANK pBank, PCXACT_PREPARE_SOUNDCUE pPrepareData, PXACTSOUNDCUE *ppCue)
{
    // No prepared-but-stopped state in the leak runtime: create+play the cue instead.
    return ((CSoundBank *)pBank)->Play(pPrepareData->dwCueIndex, pPrepareData->pSoundSource,
                                       pPrepareData->dwFlags, ppCue);
}

STDAPI IXACTEngine_SetVariable(PXACTENGINE pEngine, DWORD dwVariable, WORD wValue, DWORD /*dwApply*/)
{
    return ((CEngine *)pEngine)->SetVariable(dwVariable, wValue);
}

STDAPI IXACTEngine_GetVariable(PXACTENGINE pEngine, DWORD dwVariable, PWORD pwValue)
{
    return ((CEngine *)pEngine)->GetVariable(dwVariable, pwValue);
}

STDAPI IXACTEngine_SetParameterControl(PXACTENGINE /*pEngine*/, PCXACT_PARAMETER_CONTROL_DESC /*pParams*/)
{
    // The RPC/parameter-control engine is new in 5849 and absent from the leak; accept and ignore.
    return S_OK;
}

STDAPI IXACTEngine_SetListenerPosition(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerPosition(x, y, z, dwApply);
}

STDAPI IXACTEngine_SetListenerVelocity(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerVelocity(x, y, z, dwApply);
}

STDAPI IXACTEngine_SetListenerOrientation(PXACTENGINE pEngine, FLOAT xFront, FLOAT yFront, FLOAT zFront,
                                          FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerOrientation(xFront, yFront, zFront, xTop, yTop, zTop, dwApply);
}

STDAPI IXACTEngine_EnableHeadphones(PXACTENGINE pEngine, BOOL fEnabled)
{
    return ((CEngine *)pEngine)->EnableHeadphones(fEnabled);
}

STDAPI IXACTEngine_SetI3dl2Listener(PXACTENGINE pEngine, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetI3dl2Listener(pds3dl, dwApply);
}

STDAPI IXACTEngine_GetRealtimeData(PXACTENGINE pEngine, XACT_REALTIME_AUDIO_DATA *pData)
{
    return ((CEngine *)pEngine)->GetRealtimeData(pData);
}

STDAPI IXACTSoundBank_SelectVariation(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, PCXACT_SOUNDBANK_SELECT_VARIATION pVariation)
{
    return ((CSoundBank *)pBank)->SelectVariation(dwSoundCueIndex, pVariation);
}

STDAPI IXACTSoundBank_GetSoundCueProperties(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, PXACT_SOUNDCUE_PROPERTIES pSoundCueProperties)
{
    return ((CSoundBank *)pBank)->GetSoundCueProperties(dwSoundCueIndex, pSoundCueProperties);
}

// ---- 5849 DSP image download (renamed LoadDspImage + returns the image desc) -----------------

HRESULT STDMETHODCALLTYPE CEngine::DownloadEffectsImage(PVOID pvData, DWORD dwSize,
                                                        LPCDSEFFECTIMAGELOC pEffectLoc,
                                                        LPDSEFFECTIMAGEDESC *ppImageDesc)
{
    HRESULT hr = LoadDspImage(pvData, dwSize, pEffectLoc);
    if (ppImageDesc) {
        *ppImageDesc = SUCCEEDED(hr) ? m_pDspImageDesc : NULL;
    }
    return hr;
}

STDAPI IXACTEngine_DownloadEffectsImage(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize,
                                        LPCDSEFFECTIMAGELOC pEffectLoc, LPDSEFFECTIMAGEDESC *ppImageDesc)
{
    return ((CEngine *)pEngine)->DownloadEffectsImage(pvData, dwSize, pEffectLoc, ppImageDesc);
}

// ---- 5849 sound-source cue stop --------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSoundSource::StopSoundCues()
{
    // 5849 stops the cues playing through this source; the leak's source owns a single
    // hardware voice, so stopping that voice stops whatever those cues are rendering.
    return Stop();
}

STDAPI IXACTSoundSource_StopSoundCues(PXACTSOUNDSOURCE pSoundSource)
{
    return ((CSoundSource *)pSoundSource)->StopSoundCues();
}

// ---- IXACTWmaPlayList (RXDK 5849 uplift) -----------------------------------------------------
//
// The playlist object itself is in engine/wmaplaylist.cpp. These are the C entry points the
// public xact.h declares, plus the sound-bank factory that creates one.

STDAPI IXACTSoundBank_CreateWmaPlayList(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex,
                                        DWORD dwPlaybackFlags, PXACTWMAPLAYLIST *ppWmaPlayList)
{
    if (pBank == NULL || ppWmaPlayList == NULL) {
        return E_INVALIDARG;
    }

    *ppWmaPlayList = NULL;

    CWmaPlayList *pPlayList = new CWmaPlayList;
    if (pPlayList == NULL) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPlayList->Initialize((CSoundBank *)pBank, dwSoundCueIndex, dwPlaybackFlags);
    if (FAILED(hr)) {
        pPlayList->Release();
        return hr;
    }

    *ppWmaPlayList = (PXACTWMAPLAYLIST)pPlayList;

    return S_OK;
}

STDAPI_(ULONG) IXACTWmaPlayList_AddRef(PXACTWMAPLAYLIST pPlayList)
{
    return ((CWmaPlayList *)pPlayList)->AddRef();
}

STDAPI_(ULONG) IXACTWmaPlayList_Release(PXACTWMAPLAYLIST pPlayList)
{
    return ((CWmaPlayList *)pPlayList)->Release();
}

STDAPI IXACTWmaPlayList_Add(PXACTWMAPLAYLIST pPlayList, PCXACT_WMA_PLAYLIST_ADD pDesc,
                            PXACTWMASONG *ppSong)
{
    return ((CWmaPlayList *)pPlayList)->Add(pDesc, ppSong);
}

STDAPI IXACTWmaPlayList_Remove(PXACTWMAPLAYLIST pPlayList, PXACTWMASONG pSong)
{
    return ((CWmaPlayList *)pPlayList)->Remove(pSong);
}

STDAPI IXACTWmaPlayList_SetCurrent(PXACTWMAPLAYLIST pPlayList, PXACTWMASONG pSong)
{
    return ((CWmaPlayList *)pPlayList)->SetCurrent(pSong);
}

STDAPI IXACTWmaPlayList_Next(PXACTWMAPLAYLIST pPlayList)
{
    return ((CWmaPlayList *)pPlayList)->Next();
}

STDAPI IXACTWmaPlayList_Previous(PXACTWMAPLAYLIST pPlayList)
{
    return ((CWmaPlayList *)pPlayList)->Previous();
}

STDAPI IXACTWmaPlayList_GetCurrentSongInfo(PXACTWMAPLAYLIST pPlayList, PDWORD pdwSongLength,
                                           PWCHAR pszNameBuffer, DWORD dwBufferSize,
                                           PXACTWMASONG *ppSong)
{
    return ((CWmaPlayList *)pPlayList)->GetCurrentSongInfo(pdwSongLength, pszNameBuffer,
                                                           dwBufferSize, ppSong);
}

STDAPI IXACTWmaPlayList_GetCurrentSongInfoEx(PXACTWMAPLAYLIST pPlayList,
                                             PXACT_WMASONG_DESCRIPTION pDesc,
                                             PXACTWMASONG *ppSong)
{
    return ((CWmaPlayList *)pPlayList)->GetCurrentSongInfoEx(pDesc, ppSong);
}

STDAPI IXACTWmaPlayList_SetPlaybackBehavior(PXACTWMAPLAYLIST pPlayList, DWORD dwFlags)
{
    return ((CWmaPlayList *)pPlayList)->SetPlaybackBehavior(dwFlags);
}

STDAPI IXACTWmaPlayList_GetProperties(PXACTWMAPLAYLIST pPlayList,
                                      PXACT_WMA_PLAYLIST_PROPERTIES pProperties)
{
    return ((CWmaPlayList *)pPlayList)->GetProperties(pProperties);
}

// ---- IXACTSoundBank_PauseSoundCue (RXDK 5849 uplift) -----------------------------------------
//
// Declared by the 5849 public header and previously unimplemented, like GlobalPause. Pauses one cue
// rather than a whole category; the underlying work is the same, so both landed together.

STDAPI IXACTSoundBank_PauseSoundCue(PXACTSOUNDBANK pBank, PXACTSOUNDCUE pSoundCue, BOOL fPause)
{
    if (pBank == NULL || pSoundCue == NULL) {
        return E_INVALIDARG;
    }

    return ((CSoundCue *)pSoundCue)->Pause(fPause);
}
