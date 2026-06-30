//------------------------------------------------------------------------------
// xmvplay.c -- the retail XMVDecoder_* public API over our XMV demuxer.
//
// PHASE 1: retail container parsing + placeholder video (scrolling YUY2).
// PHASE 3 (now, alongside): AUDIO. XMVDecoder_EnableAudioStream creates a
// DirectSound stream whose WAVEFORMATEX is WAVE_FORMAT_PCM or
// WAVE_FORMAT_XBOX_ADPCM per the track's compression tag; the MCPX APU decodes
// ADPCM in hardware (libdsound supports it natively), so no software ADPCM
// decoder is needed. Each video frame we submit that frame's audio slice (a
// pointer straight into the in-memory file image -- no copy) to the stream.
//
// PHASE 2 (TODO): replace the placeholder with the ported FFmpeg WMV2 decode
// (WMV2 frame -> YUV420 -> YUY2 into the surface).
//
// The public API + ABI match shared/include/xmv.h (the retail header); the sample
// is unchanged.
//------------------------------------------------------------------------------

#include <xtl.h>
#include <d3d8.h>

// dsound.h (DirectSound streams for audio). It needs xobjbase.h's COM macros and
// a bare D3DXVECTOR3 (its 3D structs reference it) -- same prelude as the
// dsound-music sample. The bridge forward-declares IDirectSoundStream/DSMIXBINS;
// dsound.h completes them.
#include <xobjbase.h>
#ifndef __D3DX8MATH_H__
typedef struct D3DXVECTOR3 { float x, y, z; } D3DXVECTOR3;
#endif
#include <dsound.h>

#include <xmv.h>
#include "xmvdemux.h"
#include "xmvcore.h"
#include "wmv2dec.h"

#define XMV_AUDIO_PACKETS 64   // in-flight audio packet ring depth

struct XMVDecoder {
    BYTE     *file;            // whole .xmv image (audio packets point into it)
    DWORD     file_size;
    BYTE     *scratch;         // dword-reversed video frame scratch
    DWORD     scratch_size;
    XmvDemux  demux;
    DWORD     frames_shown;

    // Phase 2: leak software video kernel (I-frame/keyframe decode). NULL if the
    // geometry is unsupported (not /16) -> falls back to the placeholder render.
    XmvVideoCore *core;
    int           have_keyframe;   // set once the first keyframe has been decoded

    // WMV2 P-frame layer (increment 1: header parse + diagnostics).
    Wmv2          wmv2;
    int           wmv2_ok;

    // Audio (one enabled track for now).
    LPDIRECTSOUNDSTREAM  pStream;
    int                  audio_enabled;
    int                  audio_track;
    XBOXADPCMWAVEFORMAT  wfx;                       // big enough for PCM or ADPCM
    DWORD                pkt_status[XMV_AUDIO_PACKETS];
    int                  next_pkt;
};

// ---------------------------------------------------------------------------
// File load
// ---------------------------------------------------------------------------

static HRESULT LoadWholeFile(const char *szFileName, BYTE **ppData, DWORD *pSize)
{
    HANDLE hFile;
    DWORD  size, read, done = 0;
    BYTE  *buf;

    hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    size = GetFileSize(hFile, NULL);
    if (size == 0xFFFFFFFF || size == 0) {
        CloseHandle(hFile);
        return E_FAIL;
    }

    buf = (BYTE *)malloc(size);
    if (!buf) {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    while (done < size) {
        if (!ReadFile(hFile, buf + done, size - done, &read, NULL) || read == 0) {
            free(buf);
            CloseHandle(hFile);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        done += read;
    }

    CloseHandle(hFile);
    *ppData = buf;
    *pSize  = size;
    return S_OK;
}

// ---------------------------------------------------------------------------
// Create / close
// ---------------------------------------------------------------------------

HRESULT __stdcall XMVDecoder_CreateDecoderForFile(DWORD Flags, LPCSTR szFileName,
                                                  XMVDecoder **ppDecoder)
{
    XMVDecoder *dec;
    DWORD       max_packet;
    HRESULT     hr;
    int         rc;

    (void)Flags;
    if (!szFileName || !ppDecoder)
        return E_INVALIDARG;
    *ppDecoder = NULL;

    dec = (XMVDecoder *)malloc(sizeof(*dec));
    if (!dec)
        return E_OUTOFMEMORY;
    memset(dec, 0, sizeof(*dec));

    hr = LoadWholeFile(szFileName, &dec->file, &dec->file_size);
    if (FAILED(hr)) {
        free(dec);
        return hr;
    }

    max_packet = (dec->file_size >= 12)
        ? (dec->file[8] | (dec->file[9] << 8) | (dec->file[10] << 16) | (dec->file[11] << 24))
        : 0;
    if (max_packet == 0 || max_packet > 4 * 1024 * 1024)
        max_packet = 1024 * 1024;
    dec->scratch_size = max_packet;
    dec->scratch = (BYTE *)malloc(dec->scratch_size);
    if (!dec->scratch) {
        free(dec->file);
        free(dec);
        return E_OUTOFMEMORY;
    }

    rc = XmvDemuxOpen(&dec->demux, dec->file, dec->file_size,
                      dec->scratch, dec->scratch_size);
    if (rc != 0) {
        DbgPrint("xmv: XmvDemuxOpen failed rc=%d\n", rc);
        free(dec->scratch);
        free(dec->file);
        free(dec);
        return E_FAIL;
    }

    DbgPrint("xmv: opened %ux%u dur=%ums audio=%u (track0: comp=%u ch=%u %uHz %ubit)\n",
             dec->demux.width, dec->demux.height, dec->demux.duration_ms,
             dec->demux.audio_track_count,
             dec->demux.audio[0].compression, dec->demux.audio[0].channels,
             dec->demux.audio[0].sample_rate, dec->demux.audio[0].bits_per_sample);

    if (dec->demux.has_extradata) {
        DbgPrint("xmv: WMV2 extradata %02x %02x %02x %02x\n",
                 dec->demux.video_extradata[0], dec->demux.video_extradata[1],
                 dec->demux.video_extradata[2], dec->demux.video_extradata[3]);
    }

    // Phase 2: spin up the leak software video kernel for keyframe (I-frame)
    // decode. XINTRA8 is left disabled for now (the experiment determines whether
    // these keyframes are baseline-I or XINTRA8-coded). A NULL core (unsupported
    // geometry) degrades to the placeholder render.
    dec->core = XmvCoreCreate(dec->demux.width, dec->demux.height, 0);
    if (!dec->core)
        DbgPrint("xmv: video core unavailable (geometry %ux%u) -- placeholder render\n",
                 dec->demux.width, dec->demux.height);

    // WMV2 P-frame layer: needs the sequence extradata + the core geometry.
    if (dec->core && dec->demux.has_extradata) {
        if (Wmv2Init(&dec->wmv2, dec->core, dec->demux.video_extradata) == 0)
            dec->wmv2_ok = 1;
        else
            DbgPrint("xmv: Wmv2Init failed (P-frame decode unavailable)\n");
    }

    *ppDecoder = dec;
    return S_OK;
}

void __stdcall XMVDecoder_CloseDecoder(XMVDecoder *pDecoder)
{
    if (!pDecoder)
        return;
    DbgPrint("xmv: closing (%u frames shown)\n", pDecoder->frames_shown);
    if (pDecoder->pStream) {
        IDirectSoundStream_Flush(pDecoder->pStream);
        IDirectSoundStream_Release(pDecoder->pStream);
    }
    if (pDecoder->wmv2_ok) Wmv2Free(&pDecoder->wmv2);
    if (pDecoder->core)    XmvCoreDestroy(pDecoder->core);
    if (pDecoder->scratch) free(pDecoder->scratch);
    if (pDecoder->file)    free(pDecoder->file);
    free(pDecoder);
}

void __stdcall XMVDecoder_GetVideoDescriptor(XMVDecoder *pDecoder,
                                             XMVVIDEO_DESC *pVideoDescriptor)
{
    if (!pDecoder || !pVideoDescriptor)
        return;
    pVideoDescriptor->Width            = pDecoder->demux.width;
    pVideoDescriptor->Height           = pDecoder->demux.height;
    pVideoDescriptor->FramesPerSecond  = 0;   // derived once we decode (Phase 2)
    pVideoDescriptor->AudioStreamCount = pDecoder->demux.audio_track_count;
}

// ---------------------------------------------------------------------------
// Audio: create a DirectSound stream (PCM or XBOX ADPCM) for one track.
// ---------------------------------------------------------------------------

HRESULT __stdcall XMVDecoder_EnableAudioStream(XMVDecoder *dec, DWORD AudioStream,
                                               DWORD Flags, DSMIXBINS *pMixBins,
                                               IDirectSoundStream **ppStream)
{
    DSSTREAMDESC  desc;
    XmvAudioDesc *a;
    HRESULT       hr;

    (void)Flags; (void)pMixBins;
    if (ppStream) *ppStream = NULL;
    if (!dec)
        return E_INVALIDARG;
    if (AudioStream >= dec->demux.audio_track_count)
        return E_INVALIDARG;
    if (dec->pStream)
        return S_OK;

    a = &dec->demux.audio[AudioStream];

    memset(&dec->wfx, 0, sizeof(dec->wfx));
    if (a->compression == WAVE_FORMAT_XBOX_ADPCM) {
        // MCPX APU decodes XBOX ADPCM in hardware: 4-bit, 64 samples / 36-byte
        // block per channel (matches the demuxer's 36*ch block alignment).
        dec->wfx.wfx.wFormatTag      = WAVE_FORMAT_XBOX_ADPCM;
        dec->wfx.wfx.nChannels       = a->channels;
        dec->wfx.wfx.nSamplesPerSec  = a->sample_rate;
        dec->wfx.wfx.wBitsPerSample  = 4;
        dec->wfx.wfx.nBlockAlign     = (WORD)(a->channels * 36);
        dec->wfx.wfx.nAvgBytesPerSec = a->sample_rate / 64 * 36;
        dec->wfx.wfx.cbSize          = sizeof(dec->wfx) - sizeof(dec->wfx.wfx);
        dec->wfx.wSamplesPerBlock    = 64;
    } else {
        WORD bits = a->bits_per_sample ? a->bits_per_sample : 16;
        dec->wfx.wfx.wFormatTag      = WAVE_FORMAT_PCM;
        dec->wfx.wfx.nChannels       = a->channels;
        dec->wfx.wfx.nSamplesPerSec  = a->sample_rate;
        dec->wfx.wfx.wBitsPerSample  = bits;
        dec->wfx.wfx.nBlockAlign     = (WORD)(a->channels * (bits / 8));
        dec->wfx.wfx.nAvgBytesPerSec = a->sample_rate * dec->wfx.wfx.nBlockAlign;
        dec->wfx.wfx.cbSize          = 0;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwMaxAttachedPackets = XMV_AUDIO_PACKETS;
    desc.lpwfxFormat          = (LPWAVEFORMATEX)&dec->wfx;
    desc.lpfnCallback         = NULL;
    desc.lpvContext           = dec;
    desc.lpMixBins            = NULL;

    hr = DirectSoundCreateStream(&desc, &dec->pStream);
    if (FAILED(hr)) {
        DbgPrint("xmv: DirectSoundCreateStream failed 0x%08x\n", (unsigned)hr);
        return hr;
    }

    dec->audio_track   = (int)AudioStream;
    dec->audio_enabled = 1;
    dec->next_pkt      = 0;
    memset(dec->pkt_status, 0, sizeof(dec->pkt_status));

    DbgPrint("xmv: audio stream %u enabled (%s %uch %uHz)\n", AudioStream,
             (a->compression == WAVE_FORMAT_XBOX_ADPCM) ? "ADPCM" : "PCM",
             a->channels, a->sample_rate);

    if (ppStream) *ppStream = dec->pStream;
    return S_OK;
}

// Submit the current frame's audio slice (a pointer into the file image) to the
// stream. Recycles a completed packet slot (status != PENDING).
static void PumpAudio(XMVDecoder *dec)
{
    const BYTE *adata;
    DWORD       asize;
    int         i, slot = -1;

    if (!dec->audio_enabled || !dec->pStream)
        return;
    if (XmvDemuxAudioFrame(&dec->demux, dec->audio_track, &adata, &asize) != 1 || asize == 0)
        return;

    for (i = 0; i < XMV_AUDIO_PACKETS; i++) {
        int s = (dec->next_pkt + i) % XMV_AUDIO_PACKETS;
        if (dec->pkt_status[s] != XMEDIAPACKET_STATUS_PENDING) { slot = s; break; }
    }
    if (slot < 0)
        return;   // ring full; drop this slice (shouldn't happen with 64 slots)

    {
        XMEDIAPACKET xmp;
        memset(&xmp, 0, sizeof(xmp));
        xmp.pvBuffer   = (LPVOID)adata;     // points into dec->file (stable)
        xmp.dwMaxSize  = asize;
        xmp.pdwStatus  = &dec->pkt_status[slot];
        dec->pkt_status[slot] = XMEDIAPACKET_STATUS_PENDING;
        IDirectSoundStream_Process(dec->pStream, &xmp, NULL);
        dec->next_pkt = (slot + 1) % XMV_AUDIO_PACKETS;
    }
}

// ---------------------------------------------------------------------------
// Video: PHASE 1 placeholder render.
// ---------------------------------------------------------------------------

static void RenderPlaceholder(D3DSurface *pSurface, int keyframe, DWORD frameIdx)
{
    D3DLOCKED_RECT  rect;
    D3DSURFACE_DESC desc;
    DWORD x, y;

    if (!pSurface)
        return;

    D3DSurface_GetDesc(pSurface, &desc);
    D3DSurface_LockRect(pSurface, &rect, NULL, 0);

    for (y = 0; y < desc.Height; y++) {
        BYTE *row = (BYTE *)rect.pBits + y * rect.Pitch;
        for (x = 0; x < desc.Width; x++) {
            BYTE Y = (BYTE)((x + frameIdx * 4) & 0xFF);
            if (keyframe) Y = (BYTE)(Y | 0x80);
            row[x * 2 + 0] = Y;       // Y
            row[x * 2 + 1] = 0x80;    // U/V neutral
        }
    }

    D3DSurface_UnlockRect(pSurface);
}

HRESULT __stdcall XMVDecoder_GetNextFrame(XMVDecoder *pDecoder, IDirect3DSurface8 *pSurface,
                                          XMVRESULT *pResult, DWORD *pTimeOfFrame)
{
    const BYTE *frame;
    DWORD       size, pts = 0;
    int         keyframe = 0, rc;

    if (!pDecoder) {
        if (pResult) *pResult = XMV_FAIL;
        return E_INVALIDARG;
    }

    rc = XmvDemuxNextVideoFrame(&pDecoder->demux, &frame, &size, &keyframe, &pts);
    if (rc < 0) {
        if (pResult) *pResult = XMV_FAIL;
        return E_FAIL;
    }
    if (rc == 0) {
        if (pResult) *pResult = XMV_ENDOFFILE;
        if (pTimeOfFrame) *pTimeOfFrame = pts;
        return S_OK;
    }

    // Increment 1 diagnostic: parse + log the WMV2 P-frame header for the first
    // few P-frames. This is throwaway (P-frames are not yet decoded/rendered), so
    // it freely clobbers the bit walker; it verifies the extradata decode, the
    // ported VLC tables, and the secondary-header parse before the MB loop lands.
    if (pDecoder->wmv2_ok && !keyframe && pDecoder->frames_shown < 8) {
        int pt;
        XmvCoreSetupBits(pDecoder->core, frame);
        pt = Wmv2DecodePictureHeader(&pDecoder->wmv2);
        if (pt >= 0 && Wmv2DecodeSecondaryHeader(&pDecoder->wmv2) == 0) {
            Wmv2 *w = &pDecoder->wmv2;
            DbgPrint("wmv2: Phdr q=%d cbpidx=%d mspel=%d mv=%d dc=%d rl=%d/%d "
                     "abt(pmb=%d type=%d) nr=%d\n",
                     w->qscale, w->cbp_table_index, w->mspel, w->mv_table_index,
                     w->dc_table_index, w->rl_table_index, w->rl_chroma_table_index,
                     w->per_mb_abt, w->abt_type, w->no_rounding);
        } else {
            DbgPrint("wmv2: P header parse failed (pt=%d)\n", pt);
        }
    }

    // Phase 2: decode through the leak software video kernel. It implements only
    // baseline I-frames (P-frames are unimplemented in the leaked source), so we
    // decode each keyframe and hold it on the surface through the following
    // P-frame run -- which proves the container + WMV2 bitstream wiring. Full I+P
    // playback is the FFmpeg WMV2 port. No core (bad geometry) -> placeholder.
    if (pDecoder->core) {
        if (keyframe) {
            XmvCoreDecodeKeyframe(pDecoder->core, frame, size);
            pDecoder->have_keyframe = 1;
        }
        if (pDecoder->have_keyframe)
            XmvCoreRender(pDecoder->core, (void *)pSurface);
        else
            RenderPlaceholder((D3DSurface *)pSurface, keyframe, pDecoder->frames_shown);
    } else {
        RenderPlaceholder((D3DSurface *)pSurface, keyframe, pDecoder->frames_shown);
    }

    // Phase 3: feed this frame's audio slice to the APU stream.
    PumpAudio(pDecoder);

    if (pDecoder->frames_shown < 5) {
        DbgPrint("xmv: frame %u %s size=%u pts=%ums\n",
                 pDecoder->frames_shown, keyframe ? "[KEY]" : "", size, pts);
    }
    pDecoder->frames_shown++;

    if (pResult) *pResult = XMV_NEWFRAME;
    if (pTimeOfFrame) *pTimeOfFrame = pts;
    return S_OK;
}
