/***************************************************************************
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       xactapi.cpp
 *  Content:    XACT runtime Engine API objects and entry points.
 *  History:
 *  Date        By        Reason
 *  ====        ==        ======
 *  1/22/2002   georgioc  Created.
 *
 ****************************************************************************/

#include "xacti.h"
#include "xboxdbg.h"

#pragma comment(linker, "/merge:XACTENG_RW=XACTENG")
#pragma comment(linker, "/merge:XACTENG_URW=XACTENG")
#pragma comment(linker, "/merge:XACTENG_RD=XACTENG")
#pragma comment(linker, "/section:XACTENG,ERW")

    
INITIALIZED_CRITICAL_SECTION(g_XACTCriticalSection);

STDAPI_(ULONG) IXACTEngine_AddRef(PXACTENGINE pEngine)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->AddRef();
}

STDAPI_(ULONG) IXACTEngine_Release(PXACTENGINE pEngine)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->Release();
}


STDAPI IXACTEngine_LoadDspImage(PXACTENGINE pEngine, PVOID pvBuffer, DWORD dwSize, LPCDSEFFECTIMAGELOC pEffectLoc)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->LoadDspImage(pvBuffer, dwSize, pEffectLoc);
}

STDAPI IXACTEngine_CreateSoundSource(PXACTENGINE pEngine, DWORD dwFlags, PXACTSOUNDSOURCE *ppSoundSource)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->CreateSoundSource(dwFlags, ppSoundSource);
}

STDAPI IXACTEngine_CreateSoundBank(PXACTENGINE pEngine, PVOID pvBuffer, DWORD dwSize, PXACTSOUNDBANK *ppSoundBank)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->CreateSoundBank(pvBuffer, dwSize, ppSoundBank);
}

STDAPI IXACTEngine_RegisterWaveBank(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, PXACTWAVEBANK *ppWaveBank)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->RegisterWaveBank(pvData, dwSize, ppWaveBank);
}

// RXDK 5849 uplift: 5849 passes streaming parameters (file + packet timing) instead of the leak's
// caller-provided buffer. CEngine::RegisterStreamedWaveBank now loads the whole bank into memory, so
// the buffer args are unused (NULL/0).
STDAPI IXACTEngine_RegisterStreamedWaveBank(PXACTENGINE pEngine, PCXACT_WAVEBANK_STREAMING_PARAMETERS pParams, PXACTWAVEBANK *ppWaveBank)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->RegisterStreamedWaveBank(NULL, 0, pParams->hFile, pParams->dwOffset, ppWaveBank);
}

STDAPI IXACTEngine_UnRegisterWaveBank(PXACTENGINE pEngine, PXACTWAVEBANK pWaveBank)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->UnRegisterWaveBank(pWaveBank);
}

// RXDK 5849 uplift: 5849 added a wCategory selector (per-category master volume), and this used to
// be an empty stub -- it validated lVolume and applied nothing, so a title's volume slider did not
// move the mix. Both are real now: the volume is stored per category (or globally, for
// XACT_SOUNDBANK_CATEGORY_UNUSED), composed with the content's own volume when the sequencer sets
// it, and re-applied to cues already playing in that category.
STDAPI IXACTEngine_SetMasterVolume(PXACTENGINE pEngine, WORD wCategory, LONG lVolume)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->SetMasterVolume(wCategory, lVolume);
}

STDAPI IXACTEngine_SetListenerParameters(PXACTENGINE pEngine, LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->SetListenerParameters(pcDs3dListener, pds3dl, dwApply);
}

// RXDK 5849 uplift: 5849 added a wCategory selector here too, and this used to be an empty stub --
// it returned S_OK having paused nothing. Both are real now: CEngine::GlobalPause walks the active
// cues (and the WMA playlists, which render outside the cue path), filters on the category carried
// in the sound's bank entry, and pauses each voice in place.
//
// XACT_SOUNDBANK_CATEGORY_UNUSED means every sound, which is also what a title gets if it names a
// category no sound in the bank belongs to.
STDAPI IXACTEngine_GlobalPause(PXACTENGINE pEngine, WORD wCategory, BOOL bPause)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->GlobalPause(wCategory, bPause);
}

STDAPI IXACTEngine_RegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->RegisterNotification(pNotificationDesc);
}

STDAPI IXACTEngine_UnRegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->UnRegisterNotification(pNotificationDesc);
}

STDAPI IXACTEngine_GetNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc, PXACT_NOTIFICATION pNotification)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->GetNotification(pNotificationDesc, pNotification);
}

STDAPI IXACTEngine_FlushNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->FlushNotification(pNotificationDesc);
}

STDAPI IXACTEngine_ScheduleEvent(PXACTENGINE pEngine, XACT_TRACK_EVENT *pEventDesc, PXACTSOUNDCUE pSoundCue, DWORD dwTrackIndex)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->ScheduleEvent(pEventDesc, pSoundCue, dwTrackIndex);
}

STDAPI IXACTEngine_CommitDeferredSettings(PXACTENGINE pEngine)
{
    using namespace XACT;
    return ((CEngine *)pEngine)->CommitDeferredSettings();
}

//
// soundbank apis
//

STDAPI_(ULONG) IXACTSoundBank_AddRef(PXACTSOUNDBANK pBank)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->AddRef();
}
STDAPI_(ULONG) IXACTSoundBank_Release(PXACTSOUNDBANK pBank)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->Release();
}

STDAPI IXACTSoundBank_GetSoundCueIndexFromFriendlyName(PXACTSOUNDBANK pBank, LPCSTR lpFriendlyName, PDWORD pdwCueIndex)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->GetSoundCueIndexFromFriendlyName(lpFriendlyName, pdwCueIndex);
}

STDAPI IXACTSoundBank_Play(PXACTSOUNDBANK pBank, DWORD dwCueIndex, PXACTSOUNDSOURCE pSoundSource, DWORD dwFlags, PXACTSOUNDCUE *ppCue)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->Play(dwCueIndex, pSoundSource, dwFlags, ppCue);
}

// RXDK 5849 uplift: 5849 made PlayEx the exported entry point and turned Play into a
// header-inline that packs an XACT_PREPARE_SOUNDCUE and calls PlayEx. Adapt it back to
// the leak's 5-arg CSoundBank::Play (the extra pParameterControls field is not honored).
STDAPI IXACTSoundBank_PlayEx(PXACTSOUNDBANK pBank, PCXACT_PREPARE_SOUNDCUE pPrepareData, PXACTSOUNDCUE *ppCue)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->Play(pPrepareData->dwCueIndex, pPrepareData->pSoundSource,
                                       pPrepareData->dwFlags, ppCue);
}

STDAPI IXACTSoundBank_Stop(PXACTSOUNDBANK pBank, DWORD dwCueIndex, DWORD dwFlags, PXACTSOUNDCUE pCue)
{
    using namespace XACT;
    return ((CSoundBank *)pBank)->Stop(dwCueIndex, dwFlags, pCue);
}


//
// SoundSource apis
//

STDAPI_(ULONG) IXACTSoundSource_AddRef(PXACTSOUNDSOURCE pSoundSource)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->AddRef();
}

STDAPI_(ULONG) IXACTSoundSource_Release(PXACTSOUNDSOURCE pSoundSource)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->Release();
}

STDAPI IXACTSoundSource_SetPosition(PXACTSOUNDSOURCE pSoundSource, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetPosition(x, y, z, dwApply);
}

STDAPI IXACTSoundSource_SetAllParameters(PXACTSOUNDSOURCE pSoundSource, LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetAllParameters(pcDs3dBuffer, dwApply);
}

STDAPI IXACTSoundSource_SetConeOrientation(PXACTSOUNDSOURCE pSoundSource,FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetConeOrientation(x, y, z, dwApply);
}

STDAPI IXACTSoundSource_SetI3DL2Source(PXACTSOUNDSOURCE pSoundSource,LPCDSI3DL2BUFFER pds3db, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetI3DL2Source(pds3db,dwApply);
}

STDAPI IXACTSoundSource_SetVelocity(PXACTSOUNDSOURCE pSoundSource,FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetVelocity(x, y, z, dwApply);
}

// RXDK 5849 uplift: recovered from xacteng.lib, where both are thin forwarders
// onto CSoundSource -- see docs/5849-xact-api-recovery.md.
STDAPI IXACTSoundSource_SetPitch(PXACTSOUNDSOURCE pSoundSource, LONG lPitch)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetPitch(lPitch);
}

STDAPI IXACTSoundSource_SetFilter(PXACTSOUNDSOURCE pSoundSource, LPCDSFILTERDESC pFilterDesc)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetFilter(pFilterDesc);
}

STDAPI IXACTSoundSource_SetMode(PXACTSOUNDSOURCE pSoundSource, DWORD dwMode, DWORD dwApply)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetMode(dwMode, dwApply);
}

STDAPI IXACTSoundSource_GetStatus(PXACTSOUNDSOURCE pSoundSource, PDWORD pdwStatus)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->GetStatus(pdwStatus);
}

STDAPI IXACTSoundSource_GetProperties(PXACTSOUNDSOURCE pSoundSource, PXACT_SOUNDSOURCE_PROPERTIES pProperties)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->GetProperties(pProperties);
}

STDAPI IXACTSoundSource_SetMixBins(PXACTSOUNDSOURCE pSoundSource, LPCDSMIXBINS pMixBins)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetMixBins(pMixBins);
}

STDAPI IXACTSoundSource_SetMixBinVolumes(PXACTSOUNDSOURCE pSoundSource, LPCDSMIXBINS pMixBins)
{
    using namespace XACT;
    return ((CSoundSource *)pSoundSource)->SetMixBinVolumes(pMixBins);
}




// ==== RXDK 5849 uplift: exported C entry points (moved from engine/uplift5849.cpp) ====
using namespace XACT;
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
STDAPI IXACTEngine_DownloadEffectsImage(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize,
                                        LPCDSEFFECTIMAGELOC pEffectLoc, LPDSEFFECTIMAGEDESC *ppImageDesc)
{
    return ((CEngine *)pEngine)->DownloadEffectsImage(pvData, dwSize, pEffectLoc, ppImageDesc);
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
