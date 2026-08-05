//------------------------------------------------------------------------------
// xmvplay.c -- the retail XMVDecoder_* public API over our XMV demuxer.
//
// Container parsing, video decode and audio are all implemented. (This header
// used to describe the video as a "PHASE 1 placeholder" with the real decode as
// a TODO; that was written before the decoder landed and outlived it by a long
// way. RenderPlaceholder still exists, but only as a fallback for the frames
// before the first keyframe arrives, or if the core could not be allocated --
// it is not the normal path, and geometry is not a reason to fall back.)
//
// VIDEO: keyframes decode through the leak I-frame kernel (baseline) or the
// ported IntraX8 path when the sequence sets the j_type bit; P-frames through
// the ported WMV2 layer (picture header, macroblock loop, motion compensation,
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

    // WMV2 P-frame layer (increment 1: header parse + diagnostics).
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
// Takes ownership of `image` -- freed by CloseDecoder, or here on failure.
static HRESULT OpenDecoderOverImage(DWORD Flags, BYTE *image, DWORD image_size,
                                    XMVDecoder **ppDecoder)
{
    XMVDecoder *dec;
    DWORD       max_packet;
    int         rc;

    dec = (XMVDecoder *)malloc(sizeof(*dec));
    if (!dec) {
        free(image);
        return E_OUTOFMEMORY;
    }
    memset(dec, 0, sizeof(*dec));
    dec->loop = (Flags & XMVFLAG_FULL_LOOP) != 0;
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
        free(dec->file);
        free(dec);
        return E_OUTOFMEMORY;
    }

    rc = XmvDemuxOpen(&dec->demux, dec->file, dec->file_size,
                      dec->scratch, dec->scratch_size);
    if (rc != 0) {
        free(dec->scratch);
        free(dec->file);
        free(dec);
        return E_FAIL;
    }

    // Phase 2: spin up the leak software video kernel for keyframe (I-frame)
    // decode. XINTRA8 is left disabled for now (the experiment determines whether
    // these keyframes are baseline-I or XINTRA8-coded). A NULL core (zero-sized
    // frame, or allocation failure) degrades to the placeholder render.
    dec->core = XmvCoreCreate(dec->demux.width, dec->demux.height, 0);

    // WMV2 P-frame layer: needs the sequence extradata + the core geometry.
    if (dec->core && dec->demux.has_extradata) {
        if (Wmv2Init(&dec->wmv2, dec->core, dec->demux.video_extradata) == 0)
            dec->wmv2_ok = 1;
    }

    // X8 keyframe layer: needs only the core geometry.
    if (dec->core && Wmv2X8Init(&dec->x8, dec->core) == 0)
        dec->x8_ok = 1;

    *ppDecoder = dec;
    return S_OK;
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
    if (!pDecoder || !pAudioDescriptor ||
        AudioStream >= pDecoder->demux.audio_track_count)
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

    // Full decode. Keyframes go through the leak I-frame kernel; P-frames through
    // the ported WMV2 path (header parse + MB loop + motion comp + residual),
    // reconstructing into the building planes from the displayed (reference)
    // planes, then promoted with a swap. Decoded in sequence (P depends on the
    // previous frame). No core (zero-sized frame / allocation failure) ->
    // placeholder render.
    if (dec->core) {
        if (keyframe) {
            if (dec->wmv2_ok && dec->x8_ok) {
                // Parse the I picture header honoring the sequence options.
                // The leak kernel's own header parse predates the j_type bit:
                // when the extradata sets j_type_bit, not consuming it desyncs
                // the whole keyframe.
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
// RXDK 5849 uplift: the rest of the retail XMVDecoder_* surface.
// ---------------------------------------------------------------------------

// Retail exports these two. g_XMVInhibitDebugOutput silences the library's
// diagnostics; ours has none to silence, so it is honoured by being read
// nowhere. XMVBuildNumber identifies the library to a title that logs it.
int   g_XMVInhibitDebugOutput = 0;
DWORD XMVBuildNumber = 5849;

// Pull a whole .xmv in through the title's packet callbacks.
//
// The demuxer works over a complete in-memory image (that is how
// CreateDecoderForFile is built too), so this drains the callbacks up front
// rather than streaming. A title using this to page a movie off a slow device
// therefore pays the read cost at create time instead of during playback: the
// movie plays identically, it just does not start until the last packet has
// arrived.
HRESULT __stdcall XMVDecoder_CreateDecoderForPackets(DWORD Flags, void *pFirst4096,
                                                     DWORD Context,
                                                     PFNXMVGETNEXTPACKET pfnGetNextPacket,
                                                     PFNXMVRELEASEPREVIOUSPACKET pfnReleasePreviousPacket,
                                                     XMVDecoder **ppDecoder)
{
    BYTE     *image = NULL;
    DWORD     cap, len;
    LONGLONG  offset;
    HRESULT   hr = S_OK;

    if (!pFirst4096 || !pfnGetNextPacket || !ppDecoder)
        return E_INVALIDARG;
    *ppDecoder = NULL;

    // The first 4096 bytes come from the caller; everything after is fetched.
    cap = 64 * 1024;
    image = (BYTE *)malloc(cap);
    if (!image)
        return E_OUTOFMEMORY;
    memcpy(image, pFirst4096, 4096);
    len = 4096;
    offset = 4096;

    for (;;) {
        void  *pkt = NULL;
        DWORD  next = 0;

        hr = pfnGetNextPacket(Context, &pkt, &next);
        if (FAILED(hr))
            break;

        // No packet, or a zero step, means the stream is done.
        if (!pkt || next == 0)
            break;

        if (len + next > cap) {
            BYTE *grown;
            DWORD want = cap;

            while (want < len + next) {
                // Guard the doubling: a corrupt size must not wrap round to a
                // small capacity and turn the copy below into a heap overflow.
                if (want > 0x40000000u) {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                want *= 2;
            }
            if (FAILED(hr))
                break;

            grown = (BYTE *)realloc(image, want);
            if (!grown) {
                hr = E_OUTOFMEMORY;
                break;
            }
            image = grown;
            cap = want;
        }

        memcpy(image + len, pkt, next);
        len += next;
        offset += next;

        if (pfnReleasePreviousPacket)
            pfnReleasePreviousPacket(Context, offset, next);
    }

    if (FAILED(hr)) {
        free(image);
        return hr;
    }

    return OpenDecoderOverImage(Flags, image, len, ppDecoder);
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
    if (!pDecoder || AudioStream >= pDecoder->demux.audio_track_count)
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
// The retail library renders through a D3D overlay so this can run on a
// background thread without disturbing the title's own rendering. We do the
// same: decode into an off-screen surface and hand each new frame to the
// overlay, which the NV2A composites independently of the 3D pipeline.
HRESULT __stdcall XMVDecoder_Play(XMVDecoder *pDecoder, DWORD Flags, RECT *pRect)
{
    D3DSurface        *pSurface = NULL;
    XMVVIDEO_DESC      desc;
    HRESULT            hr = S_OK;
    int                looping;

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

    // The header's prose calls this XMVPLAY_LOOP, but no such constant is defined
    // anywhere in it; XMVFLAG_FULL_LOOP is the loop bit the library actually has.
    // Also honour the flag the decoder was created with.
    looping = ((Flags & XMVFLAG_FULL_LOOP) != 0) || pDecoder->loop;
    pDecoder->stop_after_loop = 0;
    pDecoder->stop_now        = 0;

    while (!pDecoder->stop_now) {
        XMVRESULT result = XMV_NOFRAME;
        DWORD     when = 0;

        hr = XMVDecoder_GetNextFrame(pDecoder, pSurface, &result, &when);
        if (FAILED(hr))
            break;

        if (result == XMV_NEWFRAME) {
            D3DDevice_UpdateOverlay(pSurface, NULL, pRect, FALSE, 0);
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
