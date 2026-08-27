/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

//------------------------------------------------------------------------------
// xmvplay.c -- the XMVDecoder_* public API over the XMV demuxer.
//
// Container parsing, video decode and audio are all implemented.
// RenderPlaceholder is a fallback used only for the frames before the first
// keyframe arrives, or if the decode core could not be allocated -- it is not
// the normal path, and geometry is not a reason to fall back.
//
// VIDEO: keyframes decode through the baseline I-frame kernel or the ported
// IntraX8 path when the sequence sets the j_type bit; P-frames through the
// ported WMV2 layer (picture header, macroblock loop, motion compensation,
// residual). XmvCoreCreate aligns the coded planes up to macroblock multiples,
// so a display size that is not a multiple of 16 (810x540 -> coded 816x544) is
// decoded coded-size and cropped on render -- supported, not a fallback.
//
// AUDIO: XMVDecoder_EnableAudioStream creates a
// DirectSound stream whose WAVEFORMATEX is WAVE_FORMAT_PCM or
// WAVE_FORMAT_XBOX_ADPCM per the track's compression tag; the MCPX APU decodes
// ADPCM in hardware (libdsound supports it natively), so no software ADPCM
// decoder is needed. Each video frame we submit that frame's audio slice (a
// pointer straight into the in-memory file image -- no copy) to the stream.
//
// The public API + ABI match shared/include/xmv.h.
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
#include "wmv2_x8.h"
#include "decoder.h"    // bit walker + DecodeBaselineIFrame for the I-header path

#define XMV_AUDIO_PACKETS 64   // in-flight audio packet ring depth

struct XMVDecoder {
    BYTE     *file;            // whole .xmv image (audio packets point into it)
    DWORD     file_size;
    BYTE     *scratch;         // dword-reversed video frame scratch
    DWORD     scratch_size;
    XmvDemux  demux;
    DWORD     frames_shown;
    int       loop;            // XMVFLAG_FULL_LOOP: rewind + continue at EOF

    // Software video kernel. NULL only if the frame size was zero or the
    // allocation failed -- geometry is NOT a reason (XmvCoreCreate aligns up to
    // macroblock multiples and the render crops).
    XmvVideoCore *core;
    int           have_keyframe;   // set once the first keyframe has been decoded

    // WMV2 P-frame layer (sequence + picture-header state).
    Wmv2          wmv2;
    int           wmv2_ok;

    // X8 (XINTRA8 / J-frame) keyframe layer, used when the picture header's
    // j_type bit is set.
    Wmv2X8        x8;
    int           x8_ok;

    // Audio (one enabled track for now).
    LPDIRECTSOUNDSTREAM  pStream;
    int                  audio_enabled;
    int                  audio_track;
    XBOXADPCMWAVEFORMAT  wfx;                       // big enough for PCM or ADPCM
    DWORD                pkt_status[XMV_AUDIO_PACKETS];
    int                  next_pkt;

    // A/V pacing. We decode one frame ahead and present it only once its PTS is
    // due on a wall clock (GetTickCount, ms), so playback runs at the stream's
    // real frame rate -- audio (submitted as each frame is decoded, ~1 frame
    // ahead) and video stay in lockstep. The title just presents whatever
    // XMV_NEWFRAME hands it; XMV_NOFRAME means "not yet, keep the current frame".
    int    clock_started;
    DWORD  clock_base;        // tick when frame 0 was presented
    int    held;             // a decoded frame is waiting for its presentation time
    DWORD  held_pts;         // that frame's PTS (ms)
    int    held_keyframe;

    // XMVDecoder_Play state. The Terminate* calls are documented as safe from
    // another thread, so Play only ever reads these and they are only ever set,
    // never cleared, by the terminators -- a plain volatile int is enough; no
    // lock is needed for a one-way flag.
    volatile int stop_after_loop;   // TerminateLoop: finish this pass, then stop
    volatile int stop_now;          // TerminatePlayback / TerminateImmediately
    DWORD        sync_stream;       // audio track the clock follows

    // Deferred packet-mode load. XMVDecoder_CreateDecoderForPackets is called by
    // the title BEFORE it has finished setting up its packet buffers and queued
    // the first read, so we cannot drain then. Stash the callbacks and pull the
    // whole file on first access (EnsurePacketsLoaded).
    int                         pkt_deferred;   // 1 until the drain has run
    DWORD                       pkt_flags;
    DWORD                       pkt_context;
    PFNXMVGETNEXTPACKET         pkt_get;
    PFNXMVRELEASEPREVIOUSPACKET pkt_release;
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

// Build a decoder over an .xmv image the caller has already placed in memory.
// Open the demuxer + video kernels over `image` into an already-allocated dec.
// Takes ownership of `image` -- freed here on failure (dec is left untouched
// otherwise, for the caller to free). dec->loop must already be set.
static HRESULT SetupImageIntoDecoder(XMVDecoder *dec, BYTE *image, DWORD image_size)
{
    DWORD max_packet;
    int   rc;

    dec->file = image;
    dec->file_size = image_size;

    max_packet = (dec->file_size >= 12)
        ? (dec->file[8] | (dec->file[9] << 8) | (dec->file[10] << 16) | (dec->file[11] << 24))
        : 0;
    if (max_packet == 0 || max_packet > 4 * 1024 * 1024)
        max_packet = 1024 * 1024;
    dec->scratch_size = max_packet;
    dec->scratch = (BYTE *)malloc(dec->scratch_size);
    if (!dec->scratch) {
        dec->file = NULL;
        free(image);
        return E_OUTOFMEMORY;
    }

    rc = XmvDemuxOpen(&dec->demux, dec->file, dec->file_size,
                      dec->scratch, dec->scratch_size);
    if (rc != 0) {
        free(dec->scratch);
        dec->scratch = NULL;
        dec->file = NULL;
        free(image);
        return E_FAIL;
    }

    // Spin up the software video kernel for keyframe (I-frame) decode. The
    // XINTRA8 flag is derived per-frame from the picture header, so it is left
    // disabled here. A NULL core (zero-sized frame, or allocation failure)
    // degrades to the placeholder render.
    dec->core = XmvCoreCreate(dec->demux.width, dec->demux.height, 0);

    // WMV2 P-frame layer: needs the sequence extradata + the core geometry.
    if (dec->core && dec->demux.has_extradata) {
        if (Wmv2Init(&dec->wmv2, dec->core, dec->demux.video_extradata) == 0)
            dec->wmv2_ok = 1;
    }

    // X8 keyframe layer: needs only the core geometry.
    if (dec->core && Wmv2X8Init(&dec->x8, dec->core) == 0)
        dec->x8_ok = 1;

    return S_OK;
}

// Takes ownership of `image` -- freed by CloseDecoder, or here on failure.
static HRESULT OpenDecoderOverImage(DWORD Flags, BYTE *image, DWORD image_size,
                                    XMVDecoder **ppDecoder)
{
    XMVDecoder *dec;
    HRESULT     hr;

    dec = (XMVDecoder *)malloc(sizeof(*dec));
    if (!dec) {
        free(image);
        return E_OUTOFMEMORY;
    }
    memset(dec, 0, sizeof(*dec));
    dec->loop = (Flags & XMVFLAG_FULL_LOOP) != 0;

    hr = SetupImageIntoDecoder(dec, image, image_size);
    if (FAILED(hr)) {
        free(dec);
        return hr;
    }

    *ppDecoder = dec;
    return S_OK;
}

// Drain the whole .xmv in through the title's packet callbacks (see the packet
// protocol in XMVDecoder_CreateDecoderForPackets) and open the demuxer over it.
// Runs on first access, once the title has finished OpenFileForPackets (buffers
// allocated, first read queued). Returns S_OK on success; on any failure the
// decoder is left un-loaded (demux width/height stay zero) and the caller degrades.
static HRESULT EnsurePacketsLoaded(XMVDecoder *dec)
{
    BYTE     *image;
    DWORD     cap, len;
    LONGLONG  offset;

    if (!dec->pkt_deferred)
        return dec->file ? S_OK : E_FAIL;
    dec->pkt_deferred = 0;   // only ever attempt the drain once

    cap = 256 * 1024;
    image = (BYTE *)malloc(cap);
    if (!image)
        return E_OUTOFMEMORY;
    len = 0;
    offset = 0;

    // Each packet begins with [NextPacketSize, ThisPacketSize, MaxPacketSize]; the
    // packets are contiguous in the file, so concatenating them reproduces the raw
    // .xmv the demuxer expects. GetNextPacket returns NULL while its async read is
    // still in flight, so poll rather than treating NULL as end-of-stream.
    for (;;) {
        void   *pkt = NULL;
        DWORD   thisSize = 0, nextSize, spins = 0;
        HRESULT hr;
        int     eof = 0;

        for (;;) {
            hr = dec->pkt_get(dec->pkt_context, &pkt, &thisSize);
            if (FAILED(hr)) { free(image); return hr; }
            if (pkt && thisSize) break;      // packet ready
            if (pkt && !thisSize) { eof = 1; break; }   // zero-byte read == EOF
            if (++spins > 20000) { free(image); return E_FAIL; }  // ~20s stall guard
            Sleep(1);                        // !pkt: async read still pending
        }
        if (eof)
            break;

        nextSize = (thisSize >= sizeof(DWORD)) ? ((const DWORD *)pkt)[0] : 0;

        if (len + thisSize > cap) {
            BYTE *grown;
            DWORD want = cap;
            while (want < len + thisSize) {
                if (want > 0x40000000u) { free(image); return E_OUTOFMEMORY; }
                want *= 2;
            }
            grown = (BYTE *)realloc(image, want);
            if (!grown) { free(image); return E_OUTOFMEMORY; }
            image = grown;
            cap = want;
        }
        memcpy(image + len, pkt, thisSize);
        len += thisSize;
        offset += thisSize;

        // Queue the next packet's read (no-op when nextSize == 0), then stop.
        if (dec->pkt_release)
            dec->pkt_release(dec->pkt_context, offset, nextSize);
        if (nextSize == 0)
            break;
    }

    return SetupImageIntoDecoder(dec, image, len);
}

HRESULT __stdcall XMVDecoder_CreateDecoderForFile(DWORD Flags, LPCSTR szFileName,
                                                  XMVDecoder **ppDecoder)
{
    BYTE   *image = NULL;
    DWORD   size = 0;
    HRESULT hr;

    if (!szFileName || !ppDecoder)
        return E_INVALIDARG;
    *ppDecoder = NULL;

    hr = LoadWholeFile(szFileName, &image, &size);
    if (FAILED(hr))
        return hr;

    return OpenDecoderOverImage(Flags, image, size, ppDecoder);
}

void __stdcall XMVDecoder_CloseDecoder(XMVDecoder *pDecoder)
{
    if (!pDecoder)
        return;
    if (pDecoder->pStream) {
        IDirectSoundStream_Flush(pDecoder->pStream);
        IDirectSoundStream_Release(pDecoder->pStream);
    }
    if (pDecoder->x8_ok)   Wmv2X8Free(&pDecoder->x8);
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
    EnsurePacketsLoaded(pDecoder);
    // Report the macroblock-aligned CODED size (e.g. 810x540 -> 816x544): the
    // decoder renders whole macroblocks, so surfaces created from these
    // dimensions are always large enough. Titles wanting the exact display
    // size can crop the few padding pixels (e.g. via the overlay source rect).
    pVideoDescriptor->Width            = (pDecoder->demux.width + 15) & ~15u;
    pVideoDescriptor->Height           = (pDecoder->demux.height + 15) & ~15u;
    pVideoDescriptor->FramesPerSecond  = pDecoder->demux.fps;   // probed from frame PTS deltas
    pVideoDescriptor->AudioStreamCount = pDecoder->demux.audio_track_count;
}

void __stdcall XMVDecoder_GetAudioDescriptor(XMVDecoder *pDecoder, DWORD AudioStream,
                                             XMVAUDIO_DESC *pAudioDescriptor)
{
    if (!pDecoder || !pAudioDescriptor)
        return;
    EnsurePacketsLoaded(pDecoder);
    if (AudioStream >= pDecoder->demux.audio_track_count)
        return;
    const XmvAudioDesc *a = &pDecoder->demux.audio[AudioStream];
    pAudioDescriptor->WaveFormat       = a->compression;      // WAVE_FORMAT tag (PCM / XBOX ADPCM)
    pAudioDescriptor->ChannelCount     = a->channels;
    pAudioDescriptor->SamplesPerSecond = a->sample_rate;
    pAudioDescriptor->BitsPerSample    = a->bits_per_sample;
    pAudioDescriptor->Flags            = a->flags;
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
    EnsurePacketsLoaded(dec);
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
        return hr;
    }

    dec->audio_track   = (int)AudioStream;
    dec->audio_enabled = 1;
    dec->next_pkt      = 0;
    memset(dec->pkt_status, 0, sizeof(dec->pkt_status));

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
// Video: fallback render, used before the first keyframe decodes (or if the
// core could not be allocated). Not the normal path.
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

// Decode the next frame from the demuxer into the core's planes (or placeholder),
// submit its audio, and stage it as the "held" frame awaiting its presentation
// time. Returns 1 on success, 0 at end of stream, <0 on error.
static int DecodeNextHeld(XMVDecoder *dec)
{
    const BYTE *frame;
    DWORD       size, pts = 0;
    int         keyframe = 0, rc;

    rc = XmvDemuxNextVideoFrame(&dec->demux, &frame, &size, &keyframe, &pts);
    if (rc == 0 && dec->loop) {
        // XMVFLAG_FULL_LOOP: rewind by re-opening the demuxer over the same in-memory
        // file image, reset the decode + presentation-clock state, and pull the first
        // frame again so the title never sees end-of-stream.
        XmvDemuxOpen(&dec->demux, dec->file, dec->file_size, dec->scratch, dec->scratch_size);
        dec->have_keyframe = 0;
        dec->clock_started = 0;
        dec->frames_shown  = 0;
        rc = XmvDemuxNextVideoFrame(&dec->demux, &frame, &size, &keyframe, &pts);
    }
    if (rc <= 0)
        return rc;   // 0 = EOF (no loop), <0 = error

    // Full decode. Keyframes go through the baseline I-frame kernel; P-frames
    // through the ported WMV2 path (header parse + MB loop + motion comp + residual),
    // reconstructing into the building planes from the displayed (reference)
    // planes, then promoted with a swap. Decoded in sequence (P depends on the
    // previous frame). No core (zero-sized frame / allocation failure) ->
    // placeholder render.
    if (dec->core) {
        if (keyframe) {
            if (dec->wmv2_ok && dec->x8_ok) {
                // Parse the I picture header honoring the sequence options.
                // When the extradata sets j_type_bit, the I-frame carries a
                // 1-bit j_type flag after qscale; not consuming it desyncs the
                // whole keyframe.
                XmvCoreSetupBits(dec->core, frame);
                if (ReadOneBit(dec->core) == 0) {  // I-frame marker
                    DWORD qscale;
                    SkipBits(dec->core, 7);        // buffer fullness
                    qscale = ReadBits(dec->core, 5);
                    if (dec->wmv2.j_type_bit && ReadOneBit(dec->core)) {
                        // X8 (XINTRA8 / J-frame) coded keyframe.
                        Wmv2X8DecodeFrame(&dec->x8, (int)qscale,
                                          dec->wmv2.loop_filter,
                                          frame + ((size + 3) & ~3u));
                    } else {
                        DecodeBaselineIFrame(dec->core, qscale);
                    }
                    XmvCoreSwap(dec->core);        // promote building -> displayed
                }
            } else {
                XmvCoreDecodeKeyframe(dec->core, frame, size);   // decodes + swaps
            }
            if (dec->wmv2_ok) {
                dec->wmv2.no_rounding = 1;   // I-frame: rounding base state
                Wmv2ResetMotion(&dec->wmv2);
            }
            dec->have_keyframe = 1;
        } else if (dec->have_keyframe && dec->wmv2_ok) {
            XmvCoreSetupBits(dec->core, frame);
            if (Wmv2DecodePictureHeader(&dec->wmv2) == WMV2_PICT_P &&
                Wmv2DecodeSecondaryHeader(&dec->wmv2) == 0) {
                Wmv2DecodePFrame(&dec->wmv2);
                XmvCoreSwap(dec->core);      // promote building -> displayed
            }
            // On parse failure, keep the previous displayed frame (freeze).
        }
    }

    // Feed this frame's audio slice to the APU stream now (one frame ahead of its
    // video presentation), so the stream stays primed against the wall clock.
    PumpAudio(dec);

    dec->held          = 1;
    dec->held_pts      = pts;
    dec->held_keyframe = keyframe;
    return 1;
}

HRESULT __stdcall XMVDecoder_GetNextFrame(XMVDecoder *pDecoder, IDirect3DSurface8 *pSurface,
                                          XMVRESULT *pResult, DWORD *pTimeOfFrame)
{
    DWORD now;

    if (!pDecoder) {
        if (pResult) *pResult = XMV_FAIL;
        return E_INVALIDARG;
    }

    // Packet-mode decoders defer their file load to first frame; if the drain
    // failed there is nothing to decode.
    if (FAILED(EnsurePacketsLoaded(pDecoder))) {
        if (pResult) *pResult = XMV_FAIL;
        return E_FAIL;
    }

    // Decode one frame ahead: if nothing is staged, pull and decode the next.
    if (!pDecoder->held) {
        int rc = DecodeNextHeld(pDecoder);
        if (rc < 0) {
            if (pResult) *pResult = XMV_FAIL;
            return E_FAIL;
        }
        if (rc == 0) {
            if (pResult) *pResult = XMV_ENDOFFILE;
            if (pTimeOfFrame) *pTimeOfFrame = pDecoder->demux.video_pts_ms;
            return S_OK;
        }
    }

    // Present the staged frame only once its PTS is due on the playback clock;
    // until then the title keeps showing the current frame (XMV_NOFRAME).
    if (!pDecoder->clock_started) {
        pDecoder->clock_started = 1;
        pDecoder->clock_base    = GetTickCount();
    }
    now = GetTickCount();
    if ((DWORD)(now - pDecoder->clock_base) < pDecoder->held_pts) {
        if (pResult) *pResult = XMV_NOFRAME;
        if (pTimeOfFrame) *pTimeOfFrame = pDecoder->held_pts;
        return S_OK;
    }

    // Due: render the held frame into the caller's surface and retire it.
    if (pDecoder->core && pDecoder->have_keyframe)
        XmvCoreRender(pDecoder->core, (void *)pSurface);
    else
        RenderPlaceholder((D3DSurface *)pSurface, pDecoder->held_keyframe, pDecoder->frames_shown);

    if (pResult) *pResult = XMV_NEWFRAME;
    if (pTimeOfFrame) *pTimeOfFrame = pDecoder->held_pts;
    pDecoder->held = 0;
    pDecoder->frames_shown++;
    return S_OK;
}

// ---------------------------------------------------------------------------
// The rest of the XMVDecoder_* public surface.
// ---------------------------------------------------------------------------

// Two exported globals. g_XMVInhibitDebugOutput silences the library's
// diagnostics; this build has none to silence, so it is honoured by being read
// nowhere. XMVBuildNumber identifies the library to a title that logs it.
int   g_XMVInhibitDebugOutput = 0;
DWORD XMVBuildNumber = 5849;

// Create a decoder that pulls its .xmv in through the title's packet callbacks.
//
// The title calls this from inside its OpenFileForPackets BEFORE it has allocated
// the packet buffers and queued the first read, so we cannot drain here -- doing so
// gets a NULL packet and truncates the movie to the 4K header. Instead, stash the
// callbacks and pull the whole file on first access (EnsurePacketsLoaded), by which
// point OpenFileForPackets has finished its setup. The demuxer then works over the
// complete in-memory image exactly as the file path does; the movie plays
// identically, it just does not start until the last packet has arrived.
HRESULT __stdcall XMVDecoder_CreateDecoderForPackets(DWORD Flags, void *pFirst4096,
                                                     DWORD Context,
                                                     PFNXMVGETNEXTPACKET pfnGetNextPacket,
                                                     PFNXMVRELEASEPREVIOUSPACKET pfnReleasePreviousPacket,
                                                     XMVDecoder **ppDecoder)
{
    XMVDecoder *dec;

    if (!pFirst4096 || !pfnGetNextPacket || !ppDecoder)
        return E_INVALIDARG;
    *ppDecoder = NULL;

    dec = (XMVDecoder *)malloc(sizeof(*dec));
    if (!dec)
        return E_OUTOFMEMORY;
    memset(dec, 0, sizeof(*dec));
    dec->loop        = (Flags & XMVFLAG_FULL_LOOP) != 0;
    dec->pkt_deferred = 1;
    dec->pkt_flags   = Flags;
    dec->pkt_context = Context;
    dec->pkt_get     = pfnGetNextPacket;
    dec->pkt_release = pfnReleasePreviousPacket;

    *ppDecoder = dec;
    return S_OK;
}

void __stdcall XMVDecoder_DisableAudioStream(XMVDecoder *pDecoder, DWORD AudioStream)
{
    if (!pDecoder || !pDecoder->audio_enabled || (int)AudioStream != pDecoder->audio_track)
        return;

    if (pDecoder->pStream) {
        IDirectSoundStream_Flush(pDecoder->pStream);
        IDirectSoundStream_Release(pDecoder->pStream);
        pDecoder->pStream = NULL;
    }
    pDecoder->audio_enabled = 0;
}

void __stdcall XMVDecoder_GetAudioStream(XMVDecoder *pDecoder, DWORD AudioStream,
                                         IDirectSoundStream **ppStream)
{
    if (!ppStream)
        return;
    *ppStream = NULL;

    if (!pDecoder || !pDecoder->audio_enabled || (int)AudioStream != pDecoder->audio_track)
        return;

    // A getter, not a create. The pointer is BORROWED: EnableAudioStream hands
    // the stream out the same way, the decoder owns the one reference, and
    // CloseDecoder releases it. Do not Release what this returns.
    *ppStream = pDecoder->pStream;
}

DWORD __stdcall XMVDecoder_GetSynchronizationStream(XMVDecoder *pDecoder)
{
    return pDecoder ? pDecoder->sync_stream : 0;
}

void __stdcall XMVDecoder_SetSynchronizationStream(XMVDecoder *pDecoder, DWORD AudioStream)
{
    if (!pDecoder)
        return;
    EnsurePacketsLoaded(pDecoder);
    if (AudioStream >= pDecoder->demux.audio_track_count)
        return;
    pDecoder->sync_stream = AudioStream;
}

DWORD __stdcall XMVDecoder_GetTimeFromStart(XMVDecoder *pDecoder)
{
    if (!pDecoder || !pDecoder->clock_started)
        return 0;
    return GetTickCount() - pDecoder->clock_base;
}

HRESULT __stdcall XMVDecoder_Reset(XMVDecoder *pDecoder)
{
    if (!pDecoder)
        return E_INVALIDARG;

    if (XmvDemuxOpen(&pDecoder->demux, pDecoder->file, pDecoder->file_size,
                     pDecoder->scratch, pDecoder->scratch_size) != 0)
        return E_FAIL;

    pDecoder->frames_shown    = 0;
    pDecoder->have_keyframe   = 0;
    pDecoder->held            = 0;
    pDecoder->held_pts        = 0;
    pDecoder->clock_started   = 0;
    pDecoder->stop_after_loop = 0;
    pDecoder->stop_now        = 0;

    if (pDecoder->pStream)
        IDirectSoundStream_Flush(pDecoder->pStream);

    return S_OK;
}

void __stdcall XMVDecoder_TerminateLoop(XMVDecoder *pDecoder)
{
    if (pDecoder)
        pDecoder->stop_after_loop = 1;
}

void __stdcall XMVDecoder_TerminatePlayback(XMVDecoder *pDecoder)
{
    if (pDecoder)
        pDecoder->stop_now = 1;
}

void __stdcall XMVDecoder_TerminateImmediately(XMVDecoder *pDecoder)
{
    // Documented as caller-thread-only, so unlike TerminatePlayback it may drop
    // the audio on the spot rather than letting Play wind down.
    if (!pDecoder)
        return;
    pDecoder->stop_now = 1;
    if (pDecoder->pStream)
        IDirectSoundStream_Flush(pDecoder->pStream);
}

// Play the whole movie, blocking until it ends or a terminator fires.
//
// Rendering goes through a D3D overlay so this can run on a background thread
// without disturbing the title's own rendering: decode into an off-screen
// surface and hand each new frame to the overlay, which the NV2A composites
// independently of the 3D pipeline.
HRESULT __stdcall XMVDecoder_Play(XMVDecoder *pDecoder, DWORD Flags, RECT *pRect)
{
    D3DSurface        *pSurface = NULL;
    XMVVIDEO_DESC      desc;
    HRESULT            hr = S_OK;
    int                looping;
    RECT               srcRect, dstRect;

    if (!pDecoder)
        return E_INVALIDARG;

    // "D3D must have been initialized before this API is called." The Xbox D3D8
    // free functions act on the process-wide device, so there is no device
    // pointer to take -- D3D__pDevice being null is what "not initialized" means.
    if (!D3D__pDevice)
        return E_FAIL;

    XMVDecoder_GetVideoDescriptor(pDecoder, &desc);

    hr = D3DDevice_CreateImageSurface(desc.Width, desc.Height, D3DFMT_YUY2, &pSurface);
    if (FAILED(hr))
        return hr;

    // D3DDevice_UpdateOverlay requires a non-NULL source AND destination rect. The
    // source is the whole decoded YUY2 surface; the destination is the caller's rect
    // when given, else the full screen (pRect NULL means "play full-screen").
    srcRect.left = 0; srcRect.top = 0;
    srcRect.right = (LONG)desc.Width; srcRect.bottom = (LONG)desc.Height;
    if (pRect) {
        dstRect = *pRect;
    } else {
        D3DDISPLAYMODE mode;
        D3DDevice_GetDisplayMode(&mode);
        dstRect.left = 0; dstRect.top = 0;
        dstRect.right  = (LONG)(mode.Width  ? mode.Width  : 640);
        dstRect.bottom = (LONG)(mode.Height ? mode.Height : 480);
    }

    // The header's prose calls this XMVPLAY_LOOP, but no such constant is defined
    // anywhere in it; XMVFLAG_FULL_LOOP is the loop bit the library actually has.
    // Also honour the flag the decoder was created with.
    looping = ((Flags & XMVFLAG_FULL_LOOP) != 0) || pDecoder->loop;
    pDecoder->stop_after_loop = 0;
    pDecoder->stop_now        = 0;

    // Play() owns the overlay for the length of the movie: it must be enabled
    // before UpdateOverlay will composite anything (the title using this blocking
    // interface does not touch the overlay itself, unlike the GetNextFrame path).
    D3DDevice_EnableOverlay(TRUE);

    while (!pDecoder->stop_now) {
        XMVRESULT result = XMV_NOFRAME;
        DWORD     when = 0;

        hr = XMVDecoder_GetNextFrame(pDecoder, pSurface, &result, &when);
        if (FAILED(hr))
            break;

        // Service the audio stream we own so queued packets complete and their
        // ring slots recycle. A title using this blocking path (e.g. SimpleXMV)
        // never calls DirectSoundDoWork itself -- the decoder owns audio in the
        // ppStream==NULL model -- so without this the XMV_AUDIO_PACKETS ring fills
        // after a couple of seconds, PumpAudio starts dropping slices, and audio
        // stops even though the movie plays on. Gated on audio_enabled: the stream
        // exists (DirectSoundCreateStream succeeded) so the audio system is up.
        if (pDecoder->audio_enabled)
            DirectSoundDoWork();

        if (result == XMV_NEWFRAME) {
            D3DDevice_UpdateOverlay(pSurface, &srcRect, &dstRect, FALSE, 0);
            continue;
        }

        if (result == XMV_ENDOFFILE) {
            if (!looping || pDecoder->stop_after_loop)
                break;
            if (FAILED(XMVDecoder_Reset(pDecoder)))
                break;
            continue;
        }

        if (result == XMV_FAIL) {
            hr = E_FAIL;
            break;
        }

        // XMV_NOFRAME: the next frame is decoded but not due yet. Yield rather
        // than spin -- Play owns this thread for the length of the movie.
        Sleep(1);
    }

    D3DDevice_EnableOverlay(FALSE);
    D3DSurface_Release(pSurface);

    return hr;
}
