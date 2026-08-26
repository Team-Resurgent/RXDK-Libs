/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * XACT runtime -- CEngine, the top-level runtime object a title creates.
 *
 * The engine owns the DirectSound device and pre-allocates the pools of 2D, 3D
 * and stream hardware voices that cues draw from. It registers wave and sound
 * banks (loading WMA banks by transcoding them to PCM on load, and streamed
 * banks by reading the whole file into memory), and drives per-frame work in
 * DoWork across the active cues and WMA playlists. It also implements the
 * master and per-category volume, global pause, the 3D listener, and the
 * notification system that hands playback events back to the title.
 */

#include "xacti.h"
#include "xboxdbg.h"
#include "wmabridge.h"   // WMA bank transcode-on-load


#undef DPF_FNAME
#define DPF_FNAME "XACTEngineCreateI"


EXTERN_C    XACT::CEngine* XACT::g_pEngine = NULL;

VOID
XACTEngineDoWork()
{
    using namespace XACT;

    ENTER_EXTERNAL_FUNCTION();
    CEngine*  pEngine = g_pEngine;

    if(!pEngine)
        return;

    DirectSoundDoWork();
    pEngine->DoWork();

    return;
}



HRESULT
XACTEngineCreate
(
    PXACT_RUNTIME_PARAMETERS pParams,PXACTENGINE *ppEngine   // note: pParams before ppEngine
)
{
    using namespace XACT;
    CEngine*  pEngine;
    HRESULT   hr = S_OK;
    
    DPF_ENTER();
    ENTER_EXTERNAL_FUNCTION();

#ifdef VALIDATE_PARAMETERS

    if(!ppEngine)
    {
        DPF_ERROR("Failed to supply an PXACTENGINE *");
    }

    if(!pParams)
    {
        DPF_ERROR("Failed to supply PXACT_RUNTIME_PARAMETERS");
    }

    if (!pParams->dwMax2DHwVoices)
    {

        DPF_ERROR("dwMax2dVoices must be at least max(1,number of wavebanks registered at any time)");

    }

#endif // VALIDATE_PARAMETERS
    
    DPF_ENTER();

    //
    // Check to see if the engine object exists
    //

    if(g_pEngine)
    {
        *ppEngine = g_pEngine;
        g_pEngine->AddRef();
    }
    else
    {
        hr = HRFROMP(pEngine = NEW(CEngine));

        if(SUCCEEDED(hr))
        {
            hr = pEngine->Initialize(pParams);
        }

        if(SUCCEEDED(hr))
        {
            *ppEngine = pEngine;
        }
        else
        {
            pEngine->Release();
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


using namespace XACT;


#undef DPF_FNAME
#define DPF_FNAME "CEngine::CEngine"


CEngine::CEngine
(
    void
)
{
    DPF_ENTER();

    m_dwRefCount = 1;

    //
    // Set global engine object pointer
    //

    g_pEngine = this;
    InitializeListHead(&m_lstAvailable2DBuffers);
    InitializeListHead(&m_lstAvailableStreams);
    InitializeListHead(&m_lstAvailable3DBuffers);
    InitializeListHead(&m_lstWaveBanks);
    InitializeListHead(&m_lstSoundBanks);
    InitializeListHead(&m_lstPendingNotifications);

    //
    // sequencer variables
    //

    InitializeListHead(&m_lstActiveCues);
    InitializeListHead(&m_lstPlayLists);

    // DSBVOLUME_MAX is 0 -- no attenuation -- so an untouched engine plays
    // content at exactly the volume it was authored with.
    m_lMasterVolume = DSBVOLUME_MAX;
    for (int iCat = 0; iCat < MAX_SOUND_CATEGORIES; iCat++) {
        m_alCategoryVolume[iCat] = DSBVOLUME_MAX;
    }
    KeInitializeTimer(&m_TimerObject);
    KeInitializeDpc(&m_DpcObject, DPCTimerCallBack, this);

    // Default 3D listener (front +Z, top +Y, unit factors) + zeroed variables.
    memset(&m_ds3dListener, 0, sizeof(m_ds3dListener));
    m_ds3dListener.dwSize = sizeof(m_ds3dListener);
    m_ds3dListener.vOrientFront.z = 1.0f;
    m_ds3dListener.vOrientTop.y = 1.0f;
    m_ds3dListener.flDistanceFactor = 1.0f;
    m_ds3dListener.flRolloffFactor = 1.0f;
    m_ds3dListener.flDopplerFactor = 1.0f;
    memset(m_aVariables, 0, sizeof(m_aVariables));

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::~CEngine"

CEngine::~CEngine
(
    void
)
{
    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

    PLIST_ENTRY pEntry;
    CSoundSource *pSoundSource;

    //
    // sequencer de-init
    //

    {
        AutoIrql();
        m_bAllowQueueing = FALSE;
    }

    if(m_bTimerSet){
        KeCancelTimer(&m_TimerObject);
    }

    ASSERT(m_pQueue);
    FreeAllEvents(); // From queue

    ASSERT(IsListEmpty(&m_lstActiveCues));

    DELETE(m_pQueue);

    //
    // free 2d buffers
    //

    pEntry = m_lstAvailable2DBuffers.Flink;
    while (pEntry != &m_lstAvailable2DBuffers) {

        pEntry = RemoveHeadList(&m_lstAvailable2DBuffers);
        pSoundSource = CONTAINING_RECORD(pEntry, CSoundSource, m_ListEntry);

        pSoundSource->Release();

        pEntry = m_lstAvailable2DBuffers.Flink;

    }

    //
    // free 3d buffers
    //

    pEntry = m_lstAvailable3DBuffers.Flink;
    while (pEntry != &m_lstAvailable3DBuffers) {

        pEntry = RemoveHeadList(&m_lstAvailable3DBuffers);
        pSoundSource = CONTAINING_RECORD(pEntry, CSoundSource, m_ListEntry);

        pSoundSource->Release();

        pEntry = m_lstAvailable3DBuffers.Flink;

    }

    //
    // free streams
    //

    pEntry = m_lstAvailableStreams.Flink;
    while (pEntry != &m_lstAvailableStreams) {

        pEntry = RemoveHeadList(&m_lstAvailableStreams);
        pSoundSource = CONTAINING_RECORD(pEntry, CSoundSource, m_ListEntry);

        pSoundSource->Release();

        pEntry = m_lstAvailableStreams.Flink;

    }

    if (m_pDirectSound) {
        m_pDirectSound->Release();
    }

    g_pEngine = NULL;
    DPF_INFO("XACT Engine shutdown completely");

    DPF_LEAVE_VOID();
}

ULONG CEngine::AddRef(void)
{
    _ENTER_EXTERNAL_METHOD("CEngine::AddRef");
    return ++m_dwRefCount;
}

ULONG CEngine::Release(void)
{
    _ENTER_EXTERNAL_METHOD("CEngine::Release");
    
    ASSERT(m_dwRefCount);
    m_dwRefCount--;

    if (m_dwRefCount == m_dwTotalVoiceCount){

        //
        // if the refcount equals the number of pre-allocated voices
        // it means its time to delete the engine object
        //

        delete this;
    }

    return m_dwRefCount;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::Initialize"

HRESULT CEngine::Initialize(PXACT_RUNTIME_PARAMETERS pParams)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    CSoundSource *pSoundSource;
    ENTER_EXTERNAL_METHOD();

    DSBUFFERDESC dsbd;
    DSSTREAMDESC dssd;
    WAVEFORMATEX wfx;

    DPF_ENTER();

    if (SUCCEEDED(hr)) {
        CopyMemory(&m_RuntimeParams, pParams, sizeof(XACT_RUNTIME_PARAMETERS));
        ZeroMemory( &dsbd, sizeof( DSBUFFERDESC ) );
        ZeroMemory( &dssd, sizeof( DSSTREAMDESC ) );

        hr = DirectSoundCreate(NULL,&m_pDirectSound, NULL);
    }

    if (SUCCEEDED(hr)) {
        hr = InitializeSequencer(XACT_ENGINE_MAX_CONCURRENT_EVENTS);
    }

    //
    // based on the caller supplied parameters, pre-allocate all dsound buffers and streams
    // we are about to use
    //

    if (SUCCEEDED(hr)) {

        //
        // allocate 2d buffers first
        //
        
        dsbd.dwSize = sizeof( DSBUFFERDESC );
        XAudioCreatePcmFormat(1, 8000, 8, &wfx);
        dsbd.lpwfxFormat = &wfx;
        dssd.lpwfxFormat = &wfx;
        dssd.dwMaxAttachedPackets = XACT_ENGINE_PACKETS_PER_STREAM;

        for (i=0;i<pParams->dwMax2DHwVoices-pParams->dwMaxConcurrentStreams;i++) {
    
            //
            // create the context used to track DS buffers/streams
            //
    
            hr = AllocateSoundSource(&pSoundSource);
    
            if(SUCCEEDED(hr)) {

                m_dwTotalVoiceCount++;

                //
                // add voice to linked list
                //
    
                InsertTailList(&m_lstAvailable2DBuffers,&pSoundSource->m_ListEntry);
    
                //
                // create buffer
                //
        
                hr = m_pDirectSound->CreateSoundBuffer( &dsbd, &pSoundSource->m_HwVoice.pBuffer, NULL );
    
                //
                // tell the voice the dsound flags used
                //
    
                pSoundSource->SetHwVoiceType(0);
    
            } else {

                break;

            }
    
        }

    }

    //
    // create all 3d submix voices
    //

    if (SUCCEEDED(hr)) {

        for (i=0;i<pParams->dwMax3DHwVoices;i++) {
    
            //
            // create the context used to track DS buffers/streams
            //
    
            hr = AllocateSoundSource(&pSoundSource);
    
            if(SUCCEEDED(hr)) {
    
                m_dwTotalVoiceCount++;

                //
                // add voice to linked list
                //
    
                InsertTailList(&m_lstAvailable3DBuffers,&pSoundSource->m_ListEntry);
    
                //
                // create buffer
                //
    
                dsbd.lpwfxFormat = NULL;
                dsbd.dwFlags = DSBCAPS_MIXIN | DSBCAPS_CTRL3D;

                hr = m_pDirectSound->CreateSoundBuffer( &dsbd, &pSoundSource->m_HwVoice.pBuffer, NULL );
    
                //
                // tell the voice what hw voice its associated with
                //
    
                pSoundSource->SetHwVoiceType(dsbd.dwFlags);        
    
            } else {
    
                break;
    
            }

        }

    }

    //
    // allocate streams
    //

    if (SUCCEEDED(hr)) {

        for (i=0;i<pParams->dwMaxConcurrentStreams;i++) {
    
            //
            // create the context used to track DS buffers/streams
            //
    
            hr = AllocateSoundSource(&pSoundSource);
    
            if(SUCCEEDED(hr)) {

                m_dwTotalVoiceCount++;

                //
                // add voice to linked list
                //
    
                InsertTailList(&m_lstAvailableStreams,&pSoundSource->m_ListEntry);
    
                //
                // create dsound stream
                //
        
                hr = m_pDirectSound->CreateSoundStream( &dssd, &pSoundSource->m_HwVoice.pStream, NULL);
    
                //
                // tell the voice what hw voice its associated with
                //
    
                pSoundSource->SetHwVoiceType(dssd.dwFlags);        
    
            } else {
    
                break;
    
            }

        }

    }    

    if (SUCCEEDED(hr)) {

        ASSERT(m_dwRefCount == 
            (pParams->dwMax3DHwVoices +\
             pParams->dwMax2DHwVoices)+1);

    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::AllocateSoundSource"

HRESULT CEngine::AllocateSoundSource(CSoundSource **ppSoundSource)
{
    CSoundSource *pSoundSource;
    HRESULT hr = HRFROMP(pSoundSource = NEW(CSoundSource));

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();
    
    ASSERT(ppSoundSource);

    if(SUCCEEDED(hr)) {

        hr = pSoundSource->Initialize();
        *ppSoundSource = pSoundSource;

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::FreeSoundSource"

void CEngine::FreeSoundSource(CSoundSource *pSoundSource)
{

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    ASSERT(pSoundSource);

    //
    // add the voice in the proper available list.
    //

    ASSERT(pSoundSource->m_dwRefCount == 1);

    if (pSoundSource->IsPositional()) {
        InsertTailList(&m_lstAvailable3DBuffers, &pSoundSource->m_ListEntry);
    } else {

        if (pSoundSource->m_HwVoice.pBuffer) {
            InsertTailList(&m_lstAvailable2DBuffers, &pSoundSource->m_ListEntry);
        }
        
        if (pSoundSource->m_HwVoice.pStream) {
            InsertTailList(&m_lstAvailableStreams, &pSoundSource->m_ListEntry);
        }

    }

}


#undef DPF_FNAME
#define DPF_FNAME "CEngine::DoWork"

VOID CEngine::DoWork()
{
    ENTER_EXTERNAL_METHOD();
    PLIST_ENTRY pEntry;
    CSoundCue *pCue;

    //
    // re-sync offset between cpu clock with apu sample clock
    //
    
    SetTimeOffset();
    
    //
    // tell all active cues to get busy
    //
    
    pEntry = m_lstActiveCues.Flink;
    while (pEntry != &m_lstActiveCues)
    {
        pCue = CONTAINING_RECORD(pEntry, CSoundCue, m_SeqListEntry);
        pEntry = pEntry->Flink;
        
        pCue->DoWork();        
    }

    //
    // and the same for any WMA playlist that is playing -- a playlist renders
    // itself rather than through a cue, so it needs its own pump here.
    //

    pEntry = m_lstPlayLists.Flink;
    while (pEntry != &m_lstPlayLists)
    {
        CWmaPlayList *pPlayList = CONTAINING_RECORD(pEntry, CWmaPlayList, m_ListEntry);
        pEntry = pEntry->Flink;

        pPlayList->DoWork();
    }
}


//
// WMA playlist registry.
//
// Weak references throughout: the title owns each playlist's lifetime through
// Release, and CWmaPlayList's destructor unregisters itself before tearing down.
//

VOID CEngine::RegisterPlayList(CWmaPlayList *pPlayList)
{
    InsertTailList(&m_lstPlayLists, &pPlayList->m_ListEntry);
}

VOID CEngine::UnregisterPlayList(CWmaPlayList *pPlayList)
{
    if (!IsListEmpty(&pPlayList->m_ListEntry) ||
        pPlayList->m_ListEntry.Flink != &pPlayList->m_ListEntry)
    {
        RemoveEntryList(&pPlayList->m_ListEntry);
        InitializeListHead(&pPlayList->m_ListEntry);
    }
}

CWmaPlayList * CEngine::FindPlayList(CSoundBank *pSoundBank, DWORD dwCueIndex)
{
    PLIST_ENTRY pEntry = m_lstPlayLists.Flink;

    while (pEntry != &m_lstPlayLists)
    {
        CWmaPlayList *pPlayList = CONTAINING_RECORD(pEntry, CWmaPlayList, m_ListEntry);
        pEntry = pEntry->Flink;

        if (pPlayList->GetSoundBank() == pSoundBank &&
            pPlayList->GetSoundCueIndex() == dwCueIndex)
        {
            return pPlayList;
        }
    }

    return NULL;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::GetWaveBank"

HRESULT CEngine::GetWaveBank(LPCSTR lpFriendlyName, CWaveBank **ppWaveBank)
{
    PLIST_ENTRY pEntry;
    CWaveBank *pWaveBank;
    HRESULT hr = E_FAIL;

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    *ppWaveBank = NULL;

    //
    // search wavebank list using wavebank friendly name
    //

    ASSERT(lpFriendlyName);

    if (IsListEmpty(&m_lstWaveBanks)) {
        DPF_WARNING("No wavebanks registered");
        return E_FAIL;
    }

    pEntry = m_lstWaveBanks.Flink;
    while (pEntry != &m_lstWaveBanks) {

        pWaveBank = CONTAINING_RECORD(pEntry, CWaveBank, m_ListEntry);

        if (!strncmp(pWaveBank->m_WaveBankData.pBankData->szBankName,
            lpFriendlyName,
            XACT_SOUNDBANK_WAVEBANK_FRIENDLYNAME_LENGTH)) {

            //
            // found the correct wavebank
            //

            *ppWaveBank = pWaveBank;
            hr = S_OK;

        }

        pEntry = pEntry->Flink;

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;

}


#undef DPF_FNAME
#define DPF_FNAME "CEngine::AddNotificationToPendingList"

VOID CEngine::AddNotificationToPendingList(NOTIFICATION_CONTEXT *pContext)
{

    if (pContext->bRegistered) {
        
        {
            AutoIrql();
            
            InsertTailList(&m_lstPendingNotifications,
                &pContext->ListEntry);
        }
                
        //
        // updated timestamp
        //
        
        KeQuerySystemTime((PLARGE_INTEGER)&pContext->PendingNotification.rtTimeStamp);
                               
        //
        // signal the event if present
        //
        
        if (pContext->PendingNotification.Header.hEvent) {
            SetEvent(pContext->PendingNotification.Header.hEvent);
        }
        
    }            

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::HandleNotificationRegistration"

VOID CEngine::HandleNotificationRegistration(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc, BOOL bRegister)
{
    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();
    PNOTIFICATION_CONTEXT pContext = NULL;

#ifdef VALIDATE_PARAMETERS

    if(!pNotificationDesc)
    {
        DPF_ERROR("No pNotificationDesc supplied");
    }

    if (pNotificationDesc->u.pSoundBank && pNotificationDesc->pSoundCue) {

        DPF_ERROR("You cant supply pSoundBank AND pSoundCue");
    }

    if (!pNotificationDesc->u.pSoundBank && !pNotificationDesc->pSoundCue) {

        DPF_ERROR("You must supply pSoundBank OR pSoundCue");
    }

    if ((pNotificationDesc->dwSoundCueIndex != XACT_SOUNDCUE_INDEX_UNUSED) &&
        (!pNotificationDesc->u.pSoundBank)) { 

        DPF_WARNING("YOu must supply pSoundBank if dwSoundCueIndex is specified");

    }

    //
    // validate notification type
    //

    if ((pNotificationDesc->wType) >= eXACTNotification_Max) {
        DPF_ERROR("Invalid notification type");
    }

#endif // VALIDATE_PARAMETERS

    DWORD dwType = pNotificationDesc->wType;

    //
    // first retrieve the correct notification context from a soundbank or a cue
    //

    if (pNotificationDesc->u.pSoundBank) {

        pContext = ((CSoundBank *)pNotificationDesc->u.pSoundBank)->GetNotificationContext(dwType);

    } else if (pNotificationDesc->pSoundCue) {

        //
        // tell the cue to handle the registration
        //

        pContext = ((CSoundCue *)pNotificationDesc->pSoundCue)->GetNotificationContext(dwType);

    }

    ASSERT(pContext);

    //
    // check if a notification of the same type is already registered
    //
    
#if DBG
    if (bRegister && pContext->bRegistered) {
        
        DPF_WARNING("Notification type %d already registered",
            dwType);
        
    }
    
    if (!bRegister && !pContext->bRegistered) {
        
        DPF_WARNING("Notification type %d never registered",
            dwType);
        
    }
#endif
    
    if (bRegister && !pContext->bRegistered) {
        
        //
        // this could be the first registration for this event/object combo
        //
        
        InitializeListHead(&pContext->ListEntry);
        InitializeListHead(&pContext->lstRegisteredCues);
        
    }
    
    //
    // save the notification description
    //
    
    memcpy(&pContext->PendingNotification.Header,
        pNotificationDesc,
        sizeof(XACT_NOTIFICATION_DESCRIPTION));
    
    
    pContext->bRegistered = bRegister;
    
    if (!IsListEmpty(&pContext->ListEntry)) {
        RemoveEntryList(&pContext->ListEntry);
    }

    if (pNotificationDesc->dwSoundCueIndex != XACT_SOUNDCUE_INDEX_UNUSED) {

        PCUE_INDEX_NOTIFICATION_CONTEXT pCueContext;

        //
        // create a link list of cue indices that are registred for this event type on
        // the soundbank. Then when play is called on the soundbank, using the same CueIndex,
        // turn around and register a notification with the particular cue instance
        //

        pCueContext = GetCueNotificationContext(pContext,pNotificationDesc->dwSoundCueIndex);

        if (bRegister) {                        
            
            if (!pCueContext) {
                
                pCueContext = NEW(CUE_INDEX_NOTIFICATION_CONTEXT);
                
                if (pCueContext) {
                    
                    pCueContext->dwSoundCueIndex = pNotificationDesc->dwSoundCueIndex;
                    InitializeListHead(&pCueContext->ListEntry);
                    InsertTailList(&pContext->lstRegisteredCues,
                        &pCueContext->ListEntry);

                    pCueContext->bPersist = pNotificationDesc->wFlags & XACT_FLAG_NOTIFICATION_PERSIST;
                }

            } else {

                DPF_WARNING("SoundCue index %d already registered on soundbank 0x%x",
                    pNotificationDesc->dwSoundCueIndex,
                    pContext->PendingNotification.Header.u.pSoundBank);

            }

        } else {

            if(!pCueContext) {

                DPF_WARNING("SoundCue index %d never registered on soundbank 0x%x",
                    pNotificationDesc->dwSoundCueIndex,
                    pContext->PendingNotification.Header.u.pSoundBank);

            } else {

                RemoveEntryList(&pCueContext->ListEntry);
                DELETE(pCueContext);

            }

        }

    }

    return;

}

PCUE_INDEX_NOTIFICATION_CONTEXT
CEngine::GetCueNotificationContext(PNOTIFICATION_CONTEXT pContext,DWORD dwSoundCueIndex)
{

    PLIST_ENTRY pEntry;
    PCUE_INDEX_NOTIFICATION_CONTEXT pCueContext = NULL;

    pEntry = pContext->lstRegisteredCues.Flink;
    while (pEntry && pEntry != &pContext->lstRegisteredCues) {

        pCueContext = CONTAINING_RECORD(pEntry,
            CUE_INDEX_NOTIFICATION_CONTEXT,
            ListEntry);
        
        if (pCueContext->dwSoundCueIndex == dwSoundCueIndex) {
            break;
        } else {
            pCueContext = NULL;
        }

        pEntry = pEntry->Flink;

    }

    return pCueContext;
}


VOID CEngine::IsDuplicateWaveBank(CWaveBank *pWaveBank)
{

#if DBG
    //
    // check if this wavebank has been registered before
    //
    
    CWaveBank *pExistingWaveBank;
    PLIST_ENTRY pEntry = m_lstWaveBanks.Flink;
    while (pEntry != &m_lstWaveBanks) {
        
        pExistingWaveBank = CONTAINING_RECORD(pEntry,CWaveBank,m_ListEntry);
        if (!strncmp(pExistingWaveBank->m_WaveBankData.pBankData->szBankName,
            pWaveBank->m_WaveBankData.pBankData->szBankName,
            WAVEBANK_BANKNAME_LENGTH)) {
            
            DPF_ERROR("Same wavebank (%s) has already been registered",
                pWaveBank->m_WaveBankData.pBankData->szBankName);
            
            break;
            
        }

        pEntry = pEntry->Flink;
        
    }
#endif
    
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// external methods
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#undef DPF_FNAME
#define DPF_FNAME "CEngine::LoadDspImage"

HRESULT CEngine::LoadDspImage(PVOID pvBuffer, DWORD dwSize, LPCDSEFFECTIMAGELOC pEffectLoc)
{
    HRESULT hr = S_OK;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS

    if(!pvBuffer)
    {
        DPF_ERROR("No DSP image buffer supplied");
    }

    if (dwSize == 0)
    {
        DPF_ERROR("Invalid DSP image size");
    }

#endif // VALIDATE_PARAMETERS

    //
    // Download the image, save the description
    //
    
    hr = m_pDirectSound->DownloadEffectsImage(pvBuffer, dwSize, pEffectLoc, &m_pDspImageDesc);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::CreateSoundSource"

HRESULT CEngine::CreateSoundSource(DWORD dwFlags,PXACTSOUNDSOURCE *ppSoundSource)
{
    HRESULT hr = S_OK;
    CSoundSource *pSoundSource;
    PLIST_ENTRY pEntry;

    ASSERT_IN_PASSIVE;
    DPF_ENTER();

    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS

    if(!ppSoundSource)
    {
        DPF_ERROR("No ppSoundSource supplied");
    }

    if (!IsValidSoundSourceFlags(dwFlags)) {
        DPF_ERROR("Invalid sound source flags specified");
    }

#endif // VALIDATE_PARAMETERS

    //
    // give the caller one of the pre-allocated voices based
    // on the type they are requesting
    //

    if (dwFlags & XACT_FLAG_SOUNDSOURCE_3D) {
        ASSERT(!IsListEmpty(&m_lstAvailable3DBuffers));
        pEntry = RemoveHeadList(&m_lstAvailable3DBuffers);
    } else if (dwFlags & XACT_FLAG_SOUNDSOURCE_2D) {
        ASSERT(!IsListEmpty(&m_lstAvailable2DBuffers));
        pEntry = RemoveHeadList(&m_lstAvailable2DBuffers);
    }
    
    pSoundSource = CONTAINING_RECORD(pEntry, CSoundSource, m_ListEntry);
    *ppSoundSource = pSoundSource;

    pSoundSource->AddRef();

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::AllocateStreamSoundSource"

HRESULT CEngine::AllocateStreamSoundSource(CSoundSource **ppSoundSource)
{
    HRESULT hr = S_OK;
    PLIST_ENTRY pEntry;
    CSoundSource *pSoundSource;

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    DPF_ENTER();
    ASSERT(ppSoundSource);

    //
    // Unlike a buffer voice, a stream voice is never owned by a wave bank. A bank owns the voices
    // that hold a mapping of its wave data, and a wave that is streamed is not in memory to map --
    // it arrives a packet at a time -- so the voice goes back to this pool when the cue is done
    // with it rather than to any bank's free list.
    //

    if (IsListEmpty(&m_lstAvailableStreams)) {

        DPF_ERROR("No stream voice is available; raise dwMaxConcurrentStreams");
        hr = E_FAIL;

    } else {

        pEntry = RemoveHeadList(&m_lstAvailableStreams);
        pSoundSource = CONTAINING_RECORD(pEntry, CSoundSource, m_ListEntry);

        pSoundSource->AddRef();
        *ppSoundSource = pSoundSource;

    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::CreateSoundSourceInternal"

HRESULT CEngine::CreateSoundSourceInternal(DWORD dwFlags,CWaveBank *pWaveBank, CSoundSource **ppSoundSource)
{
    HRESULT hr = S_OK;
    CSoundSource *pSoundSource = NULL;
    PXACTSOUNDSOURCE pXactSoundSource;
    CWaveBank *pWaveBank2 = NULL;

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    DPF_ENTER();
    ASSERT(ppSoundSource);

    if (pWaveBank == NULL) {

        hr = CreateSoundSource(dwFlags,&pXactSoundSource);
        pSoundSource = (CSoundSource *) pXactSoundSource;

    } else {

        //
        // try to get the voice from the appropriate wavebank
        // the wavebank will try to grab one from the engine internal lists
        // so dont look there if this fails
        //

        hr = pWaveBank->AllocateSoundSource(&pSoundSource);

        if(FAILED(hr)) {

            //
            // ok we are out of voices in the engine as well.
            // try ANY wavebank for an available voice
            //

            PLIST_ENTRY pEntry = m_lstWaveBanks.Flink;
            while (pEntry != &m_lstWaveBanks)
            {

                pWaveBank2 = CONTAINING_RECORD(pEntry, CWaveBank, m_ListEntry);
                hr = pWaveBank2->AllocateSoundSource(&pSoundSource);

                pEntry = pEntry->Flink;

                if (SUCCEEDED(hr)) {

                    //
                    // set the owner to be the wavebank they requested originally since
                    // we want the sound source to be freed to that wavebank, not where
                    // it was allocated from
                    //
                    
                    pSoundSource->SetWaveBankOwner(pWaveBank);

                    break;
                }

            }
            
        }

    }
    
    if (FAILED(hr)) {
        
        //
        // TODO: use priority to steal voices from another sound
        //

    }
    
    *ppSoundSource = pSoundSource;

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::CreateSoundBank"

HRESULT CEngine::CreateSoundBank(PVOID pvBuffer, DWORD dwSize, PXACTSOUNDBANK *ppSoundBank)
{
    HRESULT hr = S_OK;
    CSoundBank* pSoundBank;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS

    if(!pvBuffer)
    {
        DPF_ERROR("No pvBuffer supplied");
    }

    if (dwSize == 0)
    {
        DPF_ERROR("Invalid buffer size");
    }

    if(!ppSoundBank)
    {
        DPF_ERROR("No ppSoundBank supplied");
    }

#endif // VALIDATE_PARAMETERS


    //
    // create the sound bank object
    //

    hr = HRFROMP(pSoundBank = NEW(CSoundBank));
    
    if(SUCCEEDED(hr))
    {
        hr = pSoundBank->Initialize(pvBuffer,dwSize);
    }
    
    if(SUCCEEDED(hr))
    {
        *ppSoundBank = pSoundBank;
    }
    else
    {
        pSoundBank->Release();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::RegisterWaveBank"

HRESULT CEngine::RegisterWaveBank(PVOID pvData, DWORD dwSize, PXACTWAVEBANK *ppWaveBank)
{
    HRESULT hr = S_OK;
    CWaveBank *pWaveBank;

    DPF_ENTER();

    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    // If this is a WMA bank (xactbld's private RXWM container), decode every WMA entry to PCM and
    // rebuild a standard .xwb in memory, then register that. The decoded PCM buffer backs the wave
    // bank for its lifetime, so the whole bank stays resident in memory.
    {
        PVOID pvPcmBank = NULL;
        unsigned int cbPcmBank = 0;
        if (XactMaybeTranscodeWmaBank((const unsigned char *)pvData, dwSize, &pvPcmBank, &cbPcmBank))
        {
            pvData = pvPcmBank;
            dwSize = cbPcmBank;
        }
    }

#ifdef VALIDATE_PARAMETERS

    if(!pvData)
    {
        DPF_ERROR("No pvData supplied");
    }

    if (dwSize == 0)
    {
        DPF_ERROR("Invalid buffer size");
    }

    if(!ppWaveBank)
    {
        DPF_ERROR("No ppWaveBank supplied");
    }

#endif // VALIDATE_PARAMETERS

    *ppWaveBank = NULL;

    //
    // create a wrapper object to track this wavebank
    //
    
    hr = HRFROMP(pWaveBank = NEW(CWaveBank));

    if(SUCCEEDED(hr))
    {

        hr = pWaveBank->Initialize(pvData,dwSize);

    }

    if(SUCCEEDED(hr)) {

        //
        // add wave wank to our linked list of banks
        //

        IsDuplicateWaveBank(pWaveBank);

        InsertTailList(&m_lstWaveBanks,&pWaveBank->m_ListEntry);
        *ppWaveBank = (PXACTWAVEBANK) pWaveBank;

    } else {

        pWaveBank->Release();

    }

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::RegisterStreamedWaveBank"

//
// The sector size of the medium the given file is on: 512 for the hard disk, 2048 for the DVD.
// A title hands us its bank handle opened FILE_FLAG_NO_BUFFERING, and an unbuffered read has to
// obey the sector size of the medium the file is actually on, which is not something the bank's
// own alignment tells us -- a bank authored on a development kit (hard disk) still has to load
// when the title ships on a disc. Falls back to the DVD size, the coarser of the two, since a
// request aligned for the DVD is also aligned for the hard disk.
//
static DWORD XactGetSectorSize(HANDLE hFile)
{
    IO_STATUS_BLOCK          iosb;
    FILE_FS_SIZE_INFORMATION fsSize;

    if (NT_SUCCESS(NtQueryVolumeInformationFile(hFile, &iosb, &fsSize, sizeof(fsSize), FileFsSizeInformation)) &&
        fsSize.BytesPerSector != 0)
    {
        return fsSize.BytesPerSector;
    }

    return WAVEBANK_ALIGNMENT_DVD;
}

HRESULT CEngine::RegisterStreamedWaveBank(PVOID /*pvStreamingBuffer*/, DWORD /*dwSize*/, HANDLE hFileHandle, DWORD dwOffset, PXACTWAVEBANK *ppWaveBank)
{
    // Rather than truly stream, load the whole bank file into memory and register it as an
    // in-memory wave bank, so streamed cues play. The bank is fully resident for its lifetime.
    HRESULT hr = S_OK;

    DPF_ENTER();
    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

    if (ppWaveBank)
        *ppWaveBank = NULL;
    if (!hFileHandle || !ppWaveBank)
    {
        DPF_LEAVE_HRESULT(E_INVALIDARG);
        return E_INVALIDARG;
    }

    DWORD dwFileSize = GetFileSize(hFileHandle, NULL);
    if (dwFileSize == INVALID_FILE_SIZE || dwFileSize <= dwOffset)
    {
        DPF_LEAVE_HRESULT(E_FAIL);
        return E_FAIL;
    }
    DWORD dwBankSize = dwFileSize - dwOffset;

    //
    // Titles open the bank for asynchronous, unbuffered I/O (FILE_FLAG_OVERLAPPED |
    // FILE_FLAG_NO_BUFFERING), which constrains how we may read it: each read must start on a
    // sector boundary, span whole sectors, land in a sector-aligned buffer, and carry its offset
    // in an OVERLAPPED, because an asynchronous handle keeps no file pointer to seek.
    //
    // So the bank buffer is a virtual allocation rounded out to whole sectors: page-aligned, hence
    // aligned for either medium, and able to hold a bank of the several megabytes these are -- far
    // more than the pool this engine's other allocations come from is meant to serve.
    //
    DWORD dwSector    = XactGetSectorSize(hFileHandle);
    DWORD dwFileStart = dwOffset - (dwOffset % dwSector);
    DWORD dwSkip      = dwOffset - dwFileStart;
    DWORD dwNeeded    = dwSkip + dwBankSize;
    DWORD dwAllocSize = (dwNeeded + dwSector - 1) / dwSector * dwSector;

    PVOID  pvData  = VirtualAlloc(NULL, dwAllocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    HANDLE hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!pvData || !hEvent)
    {
        DPF_ERROR("Could not allocate %d bytes for the streamed bank", dwAllocSize);
        hr = E_OUTOFMEMORY;
    }

    //
    // Read whole sectors straight into the buffer. The tail read may run past the end of the file
    // and come back short, which is fine as long as the bank itself arrived.
    //
    DWORD dwDone = 0;
    while (SUCCEEDED(hr) && dwDone < dwNeeded)
    {
        OVERLAPPED ov;
        DWORD      dwRead = 0;
        DWORD      dwWant = dwAllocSize - dwDone;

        if (dwWant > 64 * 1024)
            dwWant = 64 * 1024;         // a whole number of sectors on either medium

        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEvent;
        ov.Offset = dwFileStart + dwDone;
        ResetEvent(hEvent);

        if (!ReadFile(hFileHandle, (PUCHAR)pvData + dwDone, dwWant, &dwRead, &ov) &&
            (GetLastError() != ERROR_IO_PENDING ||
             !GetOverlappedResult(hFileHandle, &ov, &dwRead, TRUE)))
        {
            DPF_ERROR("Unbuffered read of streamed bank failed at offset 0x%x (sector size %d)", ov.Offset, dwSector);
            hr = E_FAIL;
        }
        else if (dwRead == 0)
        {
            DPF_ERROR("Streamed bank is truncated: wanted %d bytes, got %d", dwNeeded, dwDone);
            hr = E_FAIL;
        }
        else
        {
            dwDone += dwRead;
        }
    }

    if (hEvent)
        CloseHandle(hEvent);

    if (SUCCEEDED(hr))
    {
        //
        // A bank that does not start the file was read from the sector boundary before it, so
        // shuffle it down to the front of the buffer.
        //
        if (dwSkip)
            memmove(pvData, (PUCHAR)pvData + dwSkip, dwBankSize);

        hr = RegisterWaveBank(pvData, dwBankSize, ppWaveBank);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Remember where the bank came from. A wave the hardware cannot play out of memory -- a WMA
        // one -- is read from the file as it plays instead, which needs the handle and the bank's
        // position in it. The handle stays the title's: it opened it, keeps it open while the bank
        // is registered, and closes it afterwards.
        //
        ((CWaveBank *)*ppWaveBank)->SetStreamingSource(hFileHandle, dwOffset);
    }

    if (FAILED(hr) && pvData)
    {
        VirtualFree(pvData, 0, MEM_RELEASE);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::UnRegisterWaveBank"

HRESULT CEngine::UnRegisterWaveBank(PXACTWAVEBANK pWaveBankInstance)
{
    HRESULT hr = S_OK;

    DPF_ENTER();
    ASSERT_IN_PASSIVE;
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS
    if(!pWaveBankInstance)
    {
        DPF_ERROR("No pWaveBankInstance supplied");
    }    
#endif // VALIDATE_PARAMETERS

    CWaveBank *pWaveBank = (CWaveBank *) pWaveBankInstance;

    //
    // remove wave bank from our registered list
    //

    RemoveEntryList(&pWaveBank->m_ListEntry);

    pWaveBank->StopAllCues();

    ASSERT(pWaveBank->m_dwRefCount == 1);

    //
    // release object
    //

    pWaveBank->Release();

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::SetMasterVolume"

//
// Sets a volume, taking a category alongside it. XACT_SOUNDBANK_CATEGORY_UNUSED
// addresses the global master; anything else addresses that category, and the
// two compose (a sound is attenuated by its category AND by the master).
//
// Xbox volumes are hundredths of a decibel, so composing attenuations is an add.
//
HRESULT CEngine::SetMasterVolume(WORD wCategory, LONG lVolume)
{
    HRESULT     hr = S_OK;
    PLIST_ENTRY pEntry;
    CSoundCue  *pCue;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS
    if((lVolume < DSBVOLUME_MIN) || (lVolume > DSBVOLUME_MAX))
    {
        DPF_ERROR("Invalid lVolume (has to be within dsound specifed volume range");
    }
#endif // VALIDATE_PARAMETERS

    if (lVolume < DSBVOLUME_MIN) lVolume = DSBVOLUME_MIN;
    if (lVolume > DSBVOLUME_MAX) lVolume = DSBVOLUME_MAX;

    if (wCategory == XACT_SOUNDBANK_CATEGORY_UNUSED)
    {
        m_lMasterVolume = lVolume;
    }
    else if (wCategory < MAX_SOUND_CATEGORIES)
    {
        m_alCategoryVolume[wCategory] = lVolume;
    }
    else
    {
        DPF_ERROR("Category %d is beyond the supported maximum (%d)",
                  wCategory, MAX_SOUND_CATEGORIES);
        hr = E_INVALIDARG;
        DPF_LEAVE_HRESULT(hr);
        return hr;
    }

    //
    // A volume is persistent state, not a momentary action like pause: it has to
    // reach sounds ALREADY playing as well as ones started later. Sounds started
    // later pick it up when the sequencer sets their volume; these are the ones
    // already going, so re-apply their remembered base against the new total.
    //
    pEntry = m_lstActiveCues.Flink;
    while (pEntry != &m_lstActiveCues)
    {
        pCue = CONTAINING_RECORD(pEntry, CSoundCue, m_SeqListEntry);
        pEntry = pEntry->Flink;

        if (wCategory != XACT_SOUNDBANK_CATEGORY_UNUSED &&
            pCue->GetCategory() != wCategory)
        {
            continue;
        }

        pCue->ReapplyVolume();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;

}


//
// Total attenuation applying to a category: its own volume plus the global
// master. Both default to DSBVOLUME_MAX (0, i.e. no attenuation), so an engine
// nobody has touched leaves content volumes exactly as authored.
//
LONG CEngine::GetCategoryAttenuation(WORD wCategory)
{
    LONG lTotal = m_lMasterVolume;

    if (wCategory != XACT_SOUNDBANK_CATEGORY_UNUSED && wCategory < MAX_SOUND_CATEGORIES)
    {
        lTotal += m_alCategoryVolume[wCategory];
    }

    if (lTotal < DSBVOLUME_MIN) lTotal = DSBVOLUME_MIN;
    if (lTotal > DSBVOLUME_MAX) lTotal = DSBVOLUME_MAX;

    return lTotal;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::SetListenerParameters"

HRESULT CEngine::SetListenerParameters(LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pcds3dl, DWORD dwApply)
{
    HRESULT hr = S_OK;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS
    if ((pcDs3dListener == NULL) && (pcds3dl == NULL)){
        DPF_ERROR("You must supply at least one set of listener parameters");
    }
#endif

    if (pcDs3dListener) {
        hr = m_pDirectSound->SetAllParameters(pcDs3dListener,dwApply);
    }

    if (pcds3dl && SUCCEEDED(hr)) {

        hr = m_pDirectSound->SetI3DL2Listener(pcds3dl,dwApply);

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::GlobalPause"

//
// Pauses or resumes sounds. wCategory selects which sounds move.
// XACT_SOUNDBANK_CATEGORY_UNUSED means all of them, which is also what a title
// gets if it passes the category of a bank built before categories existed.
//
HRESULT CEngine::GlobalPause(WORD wCategory, BOOL bPause)
{
    HRESULT     hr = S_OK;
    PLIST_ENTRY pEntry;
    CSoundCue  *pCue;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

    pEntry = m_lstActiveCues.Flink;
    while (pEntry != &m_lstActiveCues)
    {
        pCue = CONTAINING_RECORD(pEntry, CSoundCue, m_SeqListEntry);
        pEntry = pEntry->Flink;

        if (wCategory != XACT_SOUNDBANK_CATEGORY_UNUSED &&
            pCue->GetCategory() != wCategory)
        {
            continue;
        }

        // Report the last failure but pause everything that can be paused --
        // stopping halfway would leave the mix in a state no later call fixes.
        HRESULT hrCue = pCue->Pause(bPause);
        if (FAILED(hrCue))
        {
            hr = hrCue;
        }
    }

    //
    // WMA playlists render outside the cue path, so pause them alongside. They
    // carry no category of their own: a playlist is bound to a cue, so it takes
    // that cue's category.
    //
    pEntry = m_lstPlayLists.Flink;
    while (pEntry != &m_lstPlayLists)
    {
        CWmaPlayList *pPlayList = CONTAINING_RECORD(pEntry, CWmaPlayList, m_ListEntry);
        pEntry = pEntry->Flink;

        if (wCategory != XACT_SOUNDBANK_CATEGORY_UNUSED &&
            pPlayList->GetCategory() != wCategory)
        {
            continue;
        }

        pPlayList->Pause(bPause);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::RegisterNotification"

HRESULT CEngine::RegisterNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{
    HandleNotificationRegistration(pNotificationDesc, TRUE);
    return S_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::UnRegisterNotification"

HRESULT CEngine::UnRegisterNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{    
    HandleNotificationRegistration(pNotificationDesc, FALSE);
    return S_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::GetNotification"

HRESULT CEngine::GetNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc,PXACT_NOTIFICATION pNotification)
{
    HRESULT hr = S_OK;
    PNOTIFICATION_CONTEXT pContext = NULL;    

    DPF_ENTER();
    AutoIrql();
    
#ifdef VALIDATE_PARAMETERS

    if(!pNotification)
    {
        DPF_ERROR("No pNotification supplied");
    }

    if (pNotificationDesc) {

        ASSERT(!(pNotificationDesc->wFlags & XACT_MASK_NOTIFICATION_FLAGS));

        if (pNotificationDesc->u.pSoundBank && pNotificationDesc->pSoundCue) {

            DPF_ERROR("You cant specify a notification desc that has both pSoundBank and pSoundCue");

        }

        if (pNotificationDesc->dwSoundCueIndex != XACT_SOUNDCUE_INDEX_UNUSED) {

            DPF_WARNING("dwSoundCueIndex is ignored when calling this API. Set to -1");

        }
        
        if ((pNotificationDesc->wType != XACT_NOTIFICATION_TYPE_UNUSED) && 
            (pNotificationDesc->wType >= eXACTNotification_Max)) {

            DPF_ERROR("Invalid notification type specified (%d)",
                pNotificationDesc->wType);

        }

        if ((pNotificationDesc->wType == XACT_NOTIFICATION_TYPE_UNUSED) && 
            (pNotificationDesc->u.pSoundBank || pNotificationDesc->pSoundCue)) {

            DPF_ERROR("dwType must be valid if pSoundBank or pSoundCue is supplied");

        }

    }

#endif // VALIDATE_PARAMETERS

    //
    // get a notification from our linked list or soundbank,soundcue
    // based on the criteria specified
    //
    // No description at all (or TYPE_UNUSED) asks for whatever is pending, which is how a title
    // drains notifications when it registered only one kind and does not need to say which it
    // wants. Test the description pointer before dereferencing it.
    //

    if (!pNotificationDesc || pNotificationDesc->wType == XACT_NOTIFICATION_TYPE_UNUSED) {

        PLIST_ENTRY pEntry;

        //
        // retrieve the next notification regadless of type
        //

        pEntry = m_lstPendingNotifications.Flink;
        while (pEntry != &m_lstPendingNotifications) {

            pContext = CONTAINING_RECORD(pEntry,NOTIFICATION_CONTEXT,ListEntry);
            break;

        }

    } else if (pNotificationDesc->u.pSoundBank) {
        CSoundBank *pSoundBank = (CSoundBank *) pNotificationDesc->u.pSoundBank;
        pContext = pSoundBank->GetNotificationContext(pNotificationDesc->wType);        
    } else {
        CSoundCue *pSoundCue = (CSoundCue *) pNotificationDesc->pSoundCue;
        pContext = pSoundCue->GetNotificationContext(pNotificationDesc->wType);
    }

    if (pContext && IsListEmpty(&pContext->ListEntry)) {

        //
        // this context does not contain a signalled event.
        // only contexts that belong to the global notification list have 
        // pending notifications
        //

        pContext = NULL;
    }

#if DBG
    if (pContext && !pContext->bRegistered) {
        DPF_WARNING("Attempting to retrieve notification type that was never registers");
    }           
#endif

    if (pContext && pContext->bRegistered) {

        RemoveEntryList(&pContext->ListEntry);
        if (!(pContext->PendingNotification.Header.wFlags & XACT_FLAG_NOTIFICATION_PERSIST)){
            
            //
            // auto-unregister notification
            //
            
            pContext->bRegistered = FALSE;
            
        }

        //
        // copy pending notification to user-supplied buffer
        //

        memcpy(pNotification,
            &pContext->PendingNotification,
            sizeof(XACT_NOTIFICATION));
                
        
    }
   
    if (!pContext)
        hr = E_FAIL;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CEngine::FlushNotification"

HRESULT CEngine::FlushNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
{
    HRESULT hr = S_OK;
    PLIST_ENTRY pEntry;
    PNOTIFICATION_CONTEXT pContext = NULL;
    BOOL bFlush = FALSE;
    
    DPF_ENTER();
    
    AutoIrql();

#ifdef VALIDATE_PARAMETERS

    if(!pNotificationDesc)
    {
        DPF_ERROR("No pNotificationDesc supplied");
    }

    ASSERT(pNotificationDesc->wFlags & XACT_MASK_NOTIFICATION_FLAGS);

    if ((pNotificationDesc->wType != XACT_NOTIFICATION_TYPE_UNUSED) && 
        (pNotificationDesc->wType >= eXACTNotification_Max)) {

        DPF_ERROR("Invalid notification type specified (%d)",
            pNotificationDesc->wType);

    }

    if (pNotificationDesc->u.pSoundBank && pNotificationDesc->pSoundCue) {

        DPF_ERROR("You cant specify a notification desc that has both pSoundBank and pSoundCue");

    }

    if ((pNotificationDesc->dwSoundCueIndex != XACT_SOUNDCUE_INDEX_UNUSED) &&
        !pNotificationDesc->u.pSoundBank){

        DPF_WARNING("You must supply pSoundBank if dwSoundCueIndex is valid");

    }

#endif // VALIDATE_PARAMETERS

    if (pNotificationDesc->u.pSoundBank &&
        (pNotificationDesc->dwSoundCueIndex != XACT_SOUNDCUE_INDEX_UNUSED)) {
        
        PCUE_INDEX_NOTIFICATION_CONTEXT pCueContext;
        CSoundBank *pSoundBank = (CSoundBank *) pNotificationDesc->u.pSoundBank;

        //
        // soundCueIndex was specified which means we need to remove it from the soundbanks
        // list of registered cue indices
        //

        pContext = pSoundBank->GetNotificationContext(pNotificationDesc->wType);        
        pCueContext = GetCueNotificationContext(pContext,pNotificationDesc->dwSoundCueIndex);
        if (pCueContext && !(pCueContext->bPersist)) {

            RemoveEntryList(&pCueContext->ListEntry);
            DELETE(pCueContext);

        }
        
    }

    //
    // flush the appropriate pending notifications
    //

    pEntry = m_lstPendingNotifications.Flink;
    while (pEntry != &m_lstPendingNotifications) {
        
        pContext = CONTAINING_RECORD(pEntry,NOTIFICATION_CONTEXT,ListEntry);
        if (pNotificationDesc->wType == XACT_NOTIFICATION_TYPE_UNUSED)
        {
            bFlush = TRUE;

        } else if (pNotificationDesc->wType == 
            (pContext->PendingNotification.Header.wType)){

            bFlush = TRUE;
        }

        //
        // flush any notification regadless of type. If pSoundBank is supplied
        // flush all notification associated with that soundbank. Same with pSoundCue
        //
        
        if (pContext->PendingNotification.Header.u.pSoundBank &&
            (pContext->PendingNotification.Header.u.pSoundBank != pNotificationDesc->u.pSoundBank)) {
            
            bFlush = FALSE;
            
        }

        if (pContext->PendingNotification.Header.pSoundCue && 
            (pContext->PendingNotification.Header.pSoundCue != pNotificationDesc->pSoundCue)) {
            
            bFlush = FALSE;
            
        }

        if (bFlush) {

            RemoveEntryList(&pContext->ListEntry);
            if (!(pContext->PendingNotification.Header.wFlags & XACT_FLAG_NOTIFICATION_PERSIST)){

                //
                // unregister notification
                //

                pContext->bRegistered = FALSE;

            }

        }
        
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CEngine::CommitDefferedSettings"

HRESULT CEngine::CommitDeferredSettings()
{
    HRESULT hr = S_OK;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

    hr = m_pDirectSound->CommitDeferredSettings();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CEngine::ScheduleEvent"

HRESULT CEngine::ScheduleEvent(XACT_TRACK_EVENT *pEventDesc, PXACTSOUNDCUE pSoundCueObject, DWORD dwTrackIndex)
{
    HRESULT hr = S_OK;
    PTRACK_EVENT_CONTEXT pEventContext;
    PXACT_TRACK_EVENT pEvent;

    DPF_ENTER();
    ENTER_EXTERNAL_METHOD();

#ifdef VALIDATE_PARAMETERS    
    if(!pEventDesc)
    {
        DPF_ERROR("No pEvent supplied");
    }

    if (pEventDesc->Header.wType >= eXACTEvent_Max)
    {
        DPF_ERROR("Invalid Event type");
    }

    if (pSoundCueObject == NULL) {

        if (pEventDesc->Header.wType != eXACTEvent_SetEffectData) {
            DPF_ERROR("pSoundCue == NULL and eventType == eXACTEvent_SetEffectData is not a valid global event");
        }

        if (dwTrackIndex != XACT_TRACK_INDEX_UNUSED) {

            DPF_ERROR("pSoundCue must be != NULL if a valid dwTrackIndex is supplied");

        }
    }

    if (pSoundCueObject != NULL) {

        //
        // check if the event they are submitting is valid for runtime submission
        //

        if (pEventDesc->Header.wType == eXACTEvent_Play) {
            DPF_ERROR("Play is not a valid event type when submitting events through API");
        }
    }
    
#endif // VALIDATE_PARAMETERS
           
    if (SUCCEEDED(hr)) {

        //
        // pSoundCue is optional since this event can be a global event
        // such as SetEffectData that is not associated with a specific soundsource
        // If pSoundCue is supplied, based on the type of event, we modify source or target voice
        // since the user cant specify a track, only single track sounds are acceptable...
        //
        
        if (pSoundCueObject) {
            
            CSoundCue *pSoundCue = (CSoundCue *) pSoundCueObject;            
            hr = pSoundCue->ScheduleRuntimeEvent(pEventDesc,dwTrackIndex);
            
        } else {

            //
            // create a dummy track context only so we can use the 
            // CreateEventTimestamp function
            //

            TRACK_CONTEXT track;

            memset(&track,0,sizeof(track));            

            //
            // create an event context for this event
            //
            
            hr = HRFROMP(pEventContext = NEW(TRACK_EVENT_CONTEXT));
            
            if (SUCCEEDED(hr)) {
                
                hr = HRFROMP(pEvent = NEW(XACT_TRACK_EVENT));
                
            }
            
            if (SUCCEEDED(hr)) {
                
                //
                // copy user event desc
                //
                
                memcpy(pEvent,pEventDesc,sizeof(XACT_TRACK_EVENT));
                
                //
                // setup the event context
                //
                
                pEventContext->m_pEventHeader = &pEvent->Header;
                
            }
            
            track.wSamplesPerSec = 48000;
            KeQuerySystemTime((PLARGE_INTEGER)&track.rtStartTime);
            InitializeListHead(&track.lstEvents);

            CreateEventTimeStamp(&track,pEventContext);

            //
            // enqueue this event
            //

            hr = Enqueue(pEventContext);

        }
        
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}








// ---- 3D listener position, velocity and orientation -----------------------------------------
// Each updates the cached listener struct and forwards it to SetListenerParameters.

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
// EnableHeadphones and SetI3dl2Listener are thin forwarders:
// EnableHeadphones onto IDirectSound_EnableHeadphones, and SetI3dl2Listener onto
// SetListenerParameters (which forwards to IDirectSound_SetI3DL2Listener). The 3D
// math lives in libdsound, so the forward is the whole job.
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
// GetRealtimeData fills DSoundCaps via IDirectSound_GetCaps and OutputLevels via
// IDirectSound_GetOutputLevels (without resetting the peaks), bailing on the
// first failure, then fills in the engine's availability counters and the
// running allocation total.
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

// ---- DSP effects image download ---------------------------------------------------------------
// DownloadEffectsImage downloads the DSP effect image (via LoadDspImage) and returns its
// image description.

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
