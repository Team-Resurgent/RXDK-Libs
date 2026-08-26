/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Playback of one wave of a streamed wave bank. See CStreamedWave in xacti.h.
 *
 * A streamed bank can hold WMA waves, which no voice can play directly. This
 * bridges that gap: a WMA decoder XMO on one side, a hardware stream voice on
 * the other, and a two-packet ring between them, playing one wave of a streamed
 * bank.
 */

#include "xacti.h"
#include "xboxdbg.h"
#include "wavbndlr.h"

using namespace XACT;

//
//  A packet holds about a tenth of a second, so the two the voice can hold cover a fifth of a
//  second of playback -- comfortably more than the gap between two DoWork calls at any frame rate
//  a title runs at, without tying down memory for a wave that may be a brief effect.
//
#define STREAMWAVE_PACKET_MILLISECONDS  100


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::CStreamedWave"

CStreamedWave::CStreamedWave
(
    void
)
{
    DPF_ENTER();

    m_pDecoder = NULL;

    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        m_apvPacket[i] = NULL;
        m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;   // free
    }

    m_dwPacketSize = 0;
    m_fDecodedToEnd = FALSE;
    m_fToldVoice = FALSE;

    DPF_LEAVE_VOID();
}


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::~CStreamedWave"

CStreamedWave::~CStreamedWave
(
    void
)
{
    DPF_ENTER();

    if (m_pDecoder) {
        m_pDecoder->Release();
        m_pDecoder = NULL;
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        if (m_apvPacket[i]) {
            XactMemFree(m_apvPacket[i]);
            m_apvPacket[i] = NULL;
        }
    }

    DPF_LEAVE_VOID();
}


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::Initialize"

HRESULT CStreamedWave::Initialize(CWaveBank *pWaveBank, LPCWAVEBANKENTRY pEntry, LPWAVEFORMATEX pwfxDecoded)
{
    HRESULT                 hr;
    WMAXMODECODERPARAMETERS params;
    WMAXMOFileHeader        header;

    DPF_ENTER();

    ASSERT(pWaveBank);
    ASSERT(pEntry);
    ASSERT(pwfxDecoded);

    //
    // The decoder reads the wave in place, out of the file the bank was registered from: the
    // wave's ASF stream begins at its own offset there and the decoder adds that offset to every
    // read of its own accord. Nothing of the wave is copied into memory first, which is the point
    // of a streamed bank.
    //

    ZeroMemory(&params, sizeof(params));
    params.hFile = pWaveBank->GetFileHandle();
    params.dwFileOffset = pWaveBank->GetWaveFileOffset(pEntry);
    params.dwLookaheadBufferSize = 0;

    hr = XWmaDecoderCreateMediaObject(&params, &m_pDecoder);

    if (FAILED(hr)) {
        DPF_ERROR("Could not create a WMA decoder for the streamed wave (0x%08x)", hr);
        DPF_LEAVE_HRESULT(hr);
        return hr;
    }

    //
    // The decoder does no I/O until it is asked to, so its format is not known yet. Asking for the
    // stream's header is what reads it, and we need the answer now: the voice cannot be set to a
    // format we have not established, and the wave's own meta-data describes the compressed form,
    // not the PCM the voice will play.
    //

    ZeroMemory(&header, sizeof(header));

    hr = m_pDecoder->GetFileHeader(&header);

    if (SUCCEEDED(hr) && (header.dwNumChannels == 0 || header.dwSampleRate == 0)) {
        DPF_ERROR("Streamed wave reports no format (%d channels, %d Hz)",
                  header.dwNumChannels, header.dwSampleRate);
        hr = E_FAIL;
    }

    if (FAILED(hr)) {
        DPF_ERROR("Could not read the streamed wave's WMA header (0x%08x)", hr);
        DPF_LEAVE_HRESULT(hr);
        return hr;
    }

    ZeroMemory(pwfxDecoded, sizeof(*pwfxDecoded));
    pwfxDecoded->wFormatTag      = WAVE_FORMAT_PCM;
    pwfxDecoded->nChannels       = (WORD)header.dwNumChannels;
    pwfxDecoded->nSamplesPerSec  = header.dwSampleRate;
    pwfxDecoded->wBitsPerSample  = 16;                  // the WMA XMO decodes to 16-bit
    pwfxDecoded->nBlockAlign     = (WORD)(pwfxDecoded->nChannels * pwfxDecoded->wBitsPerSample / 8);
    pwfxDecoded->nAvgBytesPerSec = pwfxDecoded->nSamplesPerSec * pwfxDecoded->nBlockAlign;

    //
    // A packet has to be a whole number of sample frames, since that is the granularity both the
    // decoder produces and the voice consumes.
    //

    m_dwPacketSize = pwfxDecoded->nAvgBytesPerSec / (1000 / STREAMWAVE_PACKET_MILLISECONDS);
    m_dwPacketSize -= m_dwPacketSize % pwfxDecoded->nBlockAlign;

    if (m_dwPacketSize == 0) {
        m_dwPacketSize = pwfxDecoded->nBlockAlign;
    }

    for (DWORD i = 0; SUCCEEDED(hr) && i < PACKET_COUNT; i++) {

        m_apvPacket[i] = XactMemAlloc(m_dwPacketSize, FALSE);

        if (m_apvPacket[i] == NULL) {
            DPF_ERROR("Could not allocate a %d byte packet for the streamed wave", m_dwPacketSize);
            hr = E_OUTOFMEMORY;
        }

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::Service"

VOID CStreamedWave::Service(LPDIRECTSOUNDSTREAM pStream)
{
    ENTER_EXTERNAL_METHOD();

    ASSERT(pStream);

    if (m_pDecoder == NULL) {
        return;
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {

        XMEDIAPACKET xmp;
        DWORD        dwDecoded = 0;
        HRESULT      hr;

        //
        // PENDING means the hardware still owns this slot. Every other value means it has finished
        // with it, whether it played it, we flushed it, or it failed.
        //

        if (m_adwStatus[i] == XMEDIAPACKET_STATUS_PENDING) {
            continue;
        }

        if (m_fDecodedToEnd) {
            break;
        }

        ZeroMemory(&xmp, sizeof(xmp));
        xmp.pvBuffer = m_apvPacket[i];
        xmp.dwMaxSize = m_dwPacketSize;
        xmp.pdwCompletedSize = &dwDecoded;

        hr = m_pDecoder->Process(NULL, &xmp);

        if (FAILED(hr) || dwDecoded == 0) {
            m_fDecodedToEnd = TRUE;
            break;
        }

        ZeroMemory(&xmp, sizeof(xmp));
        xmp.pvBuffer = m_apvPacket[i];
        xmp.dwMaxSize = dwDecoded;
        xmp.pdwStatus = &m_adwStatus[i];

        m_adwStatus[i] = XMEDIAPACKET_STATUS_PENDING;

        hr = pStream->Process(&xmp, NULL);

        if (FAILED(hr)) {

            //
            // The voice would not take it -- it has no free slot of its own, most likely, so the
            // rest of the ring will not fare better. Try again next time.
            //

            m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
            break;

        }

    }

    //
    // With nothing left to send, tell the voice so once: a stream that is merely out of data
    // reports itself starved and waits for more, where one that has been told the data ended
    // plays out what it holds and stops.
    //

    if (m_fDecodedToEnd && !m_fToldVoice) {
        pStream->Discontinuity();
        m_fToldVoice = TRUE;
    }

}


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::IsFinished"

BOOL CStreamedWave::IsFinished(LPDIRECTSOUNDSTREAM pStream)
{
    ASSERT(pStream);

    if (!m_fDecodedToEnd) {
        return FALSE;
    }

    //
    // The decoder is spent, so the wave is over once the voice has given back every packet we
    // handed it.
    //

    for (DWORD i = 0; i < PACKET_COUNT; i++) {

        if (m_adwStatus[i] == XMEDIAPACKET_STATUS_PENDING) {
            return FALSE;
        }

    }

    return TRUE;
}


#undef DPF_FNAME
#define DPF_FNAME "CStreamedWave::Reset"

VOID CStreamedWave::Reset(LPDIRECTSOUNDSTREAM pStream)
{
    ENTER_EXTERNAL_METHOD();

    //
    // Flushing completes every packet the voice still holds, which both frees the ring and stops
    // the voice reading buffers we are about to give up.
    //

    if (pStream) {
        pStream->Flush();
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
    }

    m_fDecodedToEnd = FALSE;
    m_fToldVoice = FALSE;

    if (m_pDecoder) {
        m_pDecoder->Flush();
    }

}
