/***************************************************************************
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       wavebank.cpp
 *  Content:    XACT runtime wavebank object implementation
 *  History:
 *  Date        By        Reason
 *  ====        ==        ======
 *  1/27/2002   georgioc  Created.
 *
 ****************************************************************************/

#include "xacti.h"
#include "xboxdbg.h"
#include "wavbndlr.h"

using namespace XACT;

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::CWaveBank"


CWaveBank::CWaveBank
(
    void
)
{
    DPF_ENTER();
    InitializeListHead(&m_ListEntry);
    InitializeListHead(&m_lstCues);
    InitializeListHead(&m_lstAvailableSources);

    m_fPerWaveMapping = FALSE;

    m_fStreaming = FALSE;
    m_hFile = INVALID_HANDLE_VALUE;
    m_dwWaveDataSegOffset = 0;
    m_dwWaveDataFileBase = 0;

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::~CWaveBank"

CWaveBank::~CWaveBank
(
    void
)
{
    ENTER_EXTERNAL_METHOD();
    CSoundSource *pSource;

    DPF_ENTER();
    PLIST_ENTRY pEntry;

    g_pEngine->Release();
 
    m_WaveBankData.pvData = NULL;
    m_WaveBankData.dwDataSize = 0;

    pEntry = m_lstAvailableSources.Flink;
    while (pEntry != &m_lstAvailableSources){

        pSource = CONTAINING_RECORD(pEntry,CSoundSource,m_ListEntry);
        pEntry = pEntry->Flink;

        pSource->SetWaveBankOwner(NULL);

        //
        // tell dsound to release the page SGEs
        //

        if (LPDIRECTSOUNDBUFFER pBuffer = pSource->GetDSoundBuffer()) {
            pBuffer->SetBufferData(0,0);
        }

        RemoveEntryList(&pSource->m_ListEntry);
        pSource->Release();

    }

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::Initialize"

HRESULT CWaveBank::Initialize(PVOID pvData, DWORD dwSize)
{
    HRESULT hr = S_OK;
    CSoundSource *pSource;

    ENTER_EXTERNAL_METHOD();
    DPF_ENTER();

    ASSERT(g_pEngine);    
    ASSERT(pvData);
    ASSERT(dwSize);

    g_pEngine->AddRef();

    hr = AllocateSoundSource(&pSource);

    if (SUCCEEDED(hr)) {

        m_WaveBankData.pHeader = (LPWAVEBANKHEADER)pvData;
        
#ifdef VALIDATE_PARAMETERS
        //
        // Validate the header
        //
        
        if (m_WaveBankData.pHeader->dwSignature  != WAVEBANK_HEADER_SIGNATURE ||
            m_WaveBankData.pHeader->dwVersion    != WAVEBANK_HEADER_VERSION)
        {
            DPF_ERROR("Invalid wavebank header (0x%x)", pvData);
            hr = E_FAIL;
        }
#endif
        
    }

    if (SUCCEEDED(hr)) {

        //
        // The segment table locates each part of the bank, so the parts are found through it
        // rather than assumed to follow the header: the entry-name segment is only present
        // when the bank was built with names, and the wave data starts on the bank's own
        // alignment boundary, not immediately after the meta-data.
        //

        LPCWAVEBANKREGION pBankDataSeg = &m_WaveBankData.pHeader->Segments[WAVEBANK_SEGIDX_BANKDATA];
        LPCWAVEBANKREGION pMetaSeg     = &m_WaveBankData.pHeader->Segments[WAVEBANK_SEGIDX_ENTRYMETADATA];
        LPCWAVEBANKREGION pDataSeg     = &m_WaveBankData.pHeader->Segments[WAVEBANK_SEGIDX_ENTRYWAVEDATA];

        m_WaveBankData.pBankData  = (LPWAVEBANKDATA) ((PUCHAR)pvData + pBankDataSeg->dwOffset);
        m_WaveBankData.paMetaData = (LPWAVEBANKENTRY)((PUCHAR)pvData + pMetaSeg->dwOffset);
        m_WaveBankData.pvData     = (PVOID)          ((PUCHAR)pvData + pDataSeg->dwOffset);
        m_WaveBankData.dwDataSize = pDataSeg->dwLength;

        //
        // Kept for a streamed bank, whose waves are read from the file rather than from here: a
        // wave's offset is relative to this segment, so this is what turns one into a file offset.
        //

        m_dwWaveDataSegOffset = pDataSeg->dwOffset;

#ifdef VALIDATE_PARAMETERS
        if (m_WaveBankData.pBankData->dwEntryCount == 0 ||
            pDataSeg->dwOffset + pDataSeg->dwLength > dwSize)
        {
            DPF_ERROR("Invalid wavebank segments (0x%x)", pvData);
            hr = E_FAIL;
        }
#endif
        
    }

    if (SUCCEEDED(hr)) {
        
        //
        // map one 2d voice to span the entire data buffer.
        // this makes dsound pre-allocate the SGEs required by all the waves in the bank
        // and minimizes latency when playing voices pointing to this wavebank later
        //

        hr = SetBufferData(pSource->GetDSoundBuffer());

        //
        // RXDK 5849 uplift: that mapping is a latency optimization, not a requirement, and it
        // only fits banks the APU can hold in its page table in one piece -- the hardware maps
        // buffers through a table of MCPX_HW_MAX_BUFFER_PRDS pages, so a bank beyond roughly
        // eight megabytes cannot be mapped whole no matter how much memory is free. Rather than
        // refuse such a bank outright, fall back to mapping each wave as it is played: the
        // page-table cost then follows the waves actually sounding instead of the bank's size.
        //

        if (FAILED(hr)) {

            DPF_WARNING("Wavebank of %lu bytes will not fit the APU page table in one piece; mapping each wave as it plays", m_WaveBankData.dwDataSize);

            m_fPerWaveMapping = TRUE;
            hr = S_OK;

        }

    }

    //
    // the first voice is available for use by a cue since we just used it to map wave data
    // AllocateSoundSource addrefs the source one extra time to bring the voice total to at least 3
    // this way when its released it looks like it was released from a cue and it gets
    // freed back to the wavebank freelist
    //    

    if (SUCCEEDED(hr)) {

        pSource->Release();

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::MapWaveForPlayback"

HRESULT CWaveBank::MapWaveForPlayback(LPDIRECTSOUNDBUFFER pBuffer, LPCWAVEBANKENTRY pEntry, LPDWORD pdwPlayOffset)
{
    HRESULT hr;

    DPF_ENTER();

    ASSERT(pBuffer);
    ASSERT(pEntry);
    ASSERT(pdwPlayOffset);

    if (!m_fPerWaveMapping) {

        //
        // The whole bank is mapped, and every voice shares that one mapping, so the wave is
        // reached at its own offset within it.
        //

        *pdwPlayOffset = pEntry->PlayRegion.dwOffset;

        hr = SetBufferData(pBuffer);

    } else {

        //
        // Only this wave is mapped, so it sits at the front of what the voice can see. Two
        // voices playing the same wave still share a page-table run, since the heap keys those
        // on the address and length it is given.
        //

        *pdwPlayOffset = 0;

        hr = pBuffer->SetBufferData((PUCHAR)m_WaveBankData.pvData + pEntry->PlayRegion.dwOffset,
                                    pEntry->PlayRegion.dwLength);

    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::SetStreamingSource"

VOID CWaveBank::SetStreamingSource(HANDLE hFile, DWORD dwBankFileOffset)
{
    DPF_ENTER();

    ASSERT(hFile);

    m_fStreaming = TRUE;
    m_hFile = hFile;
    m_dwWaveDataFileBase = dwBankFileOffset + m_dwWaveDataSegOffset;

    DPF_LEAVE_VOID();
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::AllocateSoundSource"

HRESULT CWaveBank::AllocateSoundSource(CSoundSource **ppSource)
{

    HRESULT hr = S_OK;
    ENTER_EXTERNAL_METHOD();

    PLIST_ENTRY pEntry;
    CSoundSource *pSource;

    while (TRUE && SUCCEEDED(hr)) {

        if (!IsListEmpty(&m_lstAvailableSources)) {
            
            pEntry = RemoveHeadList(&m_lstAvailableSources);
            pSource = CONTAINING_RECORD(pEntry,CSoundSource,m_ListEntry);
            
        } else {
            
            //
            // get one from the engine
            //
            
            hr = g_pEngine->CreateSoundSourceInternal(XACT_FLAG_SOUNDSOURCE_2D,NULL,&pSource);        
            
        }
        
        if (SUCCEEDED(hr)){
            
            pSource->AddRef();
            pSource->SetWaveBankOwner(this);
            *ppSource = pSource;
            
        }
        
        if (SUCCEEDED(hr) && pSource->IsPlaying()) {

            DPF_WARNING("Voice form available list was still playing in hw, attempting re-alloc of new one");

            //
            // hmm the free voice we got is still playing...
            // we d rather allocate a new one and leave this one alone for now
            //
            
            pSource->Release();
            
        } else {
            break;
        }

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;

}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::FreeSoundSource"

VOID CWaveBank::FreeSoundSource(CSoundSource *pSource)
{

    ENTER_EXTERNAL_METHOD();

    ASSERT(pSource);
    ASSERT(pSource->m_pWaveBankOwner == this);

    InsertTailList(&m_lstAvailableSources,&pSource->m_ListEntry);

}



#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::AddCueToList"

VOID CWaveBank::AddCueToList(PWAVEBANK_CUE_CONTEXT pEntry)
{
    ENTER_EXTERNAL_METHOD();
    InsertTailList(&m_lstCues,
        &pEntry->ListEntry);

}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::RemoveCueFromList"

VOID CWaveBank::RemoveCueFromList(PWAVEBANK_CUE_CONTEXT pEntry)
{
    ENTER_EXTERNAL_METHOD();
    RemoveEntryList(&pEntry->ListEntry);
}

#undef DPF_FNAME
#define DPF_FNAME "CWaveBank::StopAllCues"

VOID CWaveBank::StopAllCues()
{
    ENTER_EXTERNAL_METHOD();
    CSoundCue *pCue = NULL;
    PWAVEBANK_CUE_CONTEXT pCueEntry;

    //
    // stop all cues associated with this wavebank
    //

    PLIST_ENTRY pEntry = m_lstCues.Flink;
    while (pEntry != &m_lstCues){

        pCueEntry = CONTAINING_RECORD(pEntry,WAVEBANK_CUE_CONTEXT,ListEntry);
        pCue = pCueEntry->pSoundCue;
        pEntry = pEntry->Flink;

        DPF_WARNING("You are un-registering wavebank 0x%x still referenced by cue %s.\n"\
            "        This can cause breakup and glitching. Cue %x is no longer valid",
            this,
            pCue->GetFriendlyName(),
            pCue);

        pCue->Stop(XACT_FLAG_SOUNDCUE_SYNCHRONOUS | XACT_FLAG_SOUNDCUE_AUTORELEASE);

    }

}