//------------------------------------------------------------------------------
// DirectSound OGG music player -- RXDK libdsound + stb_vorbis sample.
//
// Reads D:\media\music.ogg and STREAMS it through libdsound (the Xbox MCPX APU):
// stb_vorbis decodes into a small looping double-buffer that's refilled behind
// the play cursor, so a full-length track plays without decoding it all into RAM.
//
// media\music.ogg is deployed alongside the XBE (build-iso.ps1 copies any
// sample's media\ folder to xe:\DEVKIT\xbetest\<sample>\media via xbcp).
//------------------------------------------------------------------------------

// common.h first: pulls the xapi/xtl/NT env (NT_INCLUDED + base Win32 types).
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <guiddef.h>

// dsound.h builds COM vtables with the STDMETHOD* calling-convention macros that
// a full XDK supplies via xtl.h; this light C title defines just that prelude
// (same set the libdsound build provides via site/bridge_dsound.h).
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE   __stdcall
#define STDMETHODVCALLTYPE  __cdecl
#define STDAPICALLTYPE      __stdcall
#define STDAPIVCALLTYPE     __cdecl
#endif
#ifndef EXTERN_C
#define EXTERN_C extern
#endif
#ifndef STDAPI
#define STDAPI       EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(t)   EXTERN_C t STDAPICALLTYPE
#endif
// dsound.h's DECLARE_INTERFACE vtables need xobjbase.h's COM macros; its 3D-audio
// structs use D3DXVECTOR3 (we only need the bare type -- see d3d8-textures for why
// we don't pull all of d3dx8math.h here).
#include <xobjbase.h>
#ifndef __D3DX8MATH_H__
typedef struct D3DXVECTOR3 { float x, y, z; } D3DXVECTOR3;
#endif
#include <dsound.h>

// --- stb_vorbis (compiled as its own TU; STB_VORBIS_NO_STDIO) -----------------
typedef struct stb_vorbis stb_vorbis;
typedef struct {
    unsigned int sample_rate;
    int          channels;
    unsigned int setup_memory_required;
    unsigned int setup_temp_memory_required;
    unsigned int temp_memory_required;
    int          max_frame_size;
} stb_vorbis_info;
// stb_vorbis allocation arena. Without it, stb_vorbis allocates per-frame decode
// temporaries (inverse_mdct / decode_residue, several blocksize-sized float
// arrays down a deep call chain) with alloca() -- which overflows the modest
// title thread stack (XBE SizeOfStack, 64 KiB fallback) on the FIRST decoded
// frame and faults during priming. Passing a heap buffer makes stb_vorbis carve
// all memory (setup + temp) from it instead. Layout must match stb_vorbis.c's
// stb_vorbis_alloc { char *buffer; int length; }.
typedef struct { char *alloc_buffer; int alloc_buffer_length_in_bytes; } rxdk_stb_vorbis_alloc;
#define VORBIS_ARENA_BYTES (1024 * 1024)   // setup codebooks + peak temp; bump if open returns VORBIS_outofmem
extern stb_vorbis     *stb_vorbis_open_memory(const unsigned char *data, int len, int *error, void *alloc);
extern stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);
extern int             stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
extern int             stb_vorbis_seek_start(stb_vorbis *f);
extern void            stb_vorbis_close(stb_vorbis *f);

// dsound's COM classes have pure-virtual methods; provide the ABI stub the C++
// runtime would (this C title links no libcpp). Symbol: ___cxa_pure_virtual.
void __cxa_pure_virtual(void) { for (;;) {} }

// Stream playback model (mirrors PrometheOS audioPlayer.cpp): decode stb_vorbis
// into cached malloc'd packets and submit them to an IDirectSoundStream via
// XMEDIAPACKET/Process. The APU DMAs from our cached buffers, so we never write
// PCM into the uncached/write-combined hardware buffer (which faulted under the
// CreateSoundBuffer + Lock approach).
#define AUDIO_PACKETS         64
#define AUDIO_OUTPUT_BUF_SIZE 8192   // bytes per packet

static stb_vorbis *g_vorbis;
static int         g_channels;

// --- read a whole file from the title's D: into a malloc'd buffer -------------
static unsigned char *read_file(const char *path, DWORD *out_len)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        DbgPrint("dsound-music: CreateFile failed for %s\n", path);
        return NULL;
    }
    DWORD len = GetFileSize(h, NULL);
    unsigned char *buf = (unsigned char *)malloc(len);
    DWORD got = 0;
    if (buf && ReadFile(h, buf, len, &got, NULL) && got == len) {
        CloseHandle(h);
        *out_len = len;
        return buf;
    }
    DbgPrint("dsound-music: ReadFile failed (len=%u got=%u)\n", (unsigned)len, (unsigned)got);
    CloseHandle(h);
    free(buf);
    return NULL;
}

int main(void)
{
    LPDIRECTSOUND       pDS = NULL;
    LPDIRECTSOUNDSTREAM stream = NULL;
    WAVEFORMATEX        wfx;
    DSSTREAMDESC        sd;
    DSMIXBINVOLUMEPAIR  mixPairs[6];
    DSMIXBINS           mixBins;
    stb_vorbis_info     info;
    unsigned char      *ogg;
    DWORD               oggLen = 0;
    int                 err = 0;
    HRESULT             hr;
    char               *decodeBuffer;
    DWORD              *packetStatus;
    DWORD              *completedSize;
    int                 i;
    int                 submitted = 0;

    xapi_smoke_trace_line("dsound-music start");

    // 1. Load the OGG and open the decoder (keep the file buffer alive -- the
    //    decoder reads from it). Streaming means we never hold the whole PCM.
    ogg = read_file("D:\\media\\music.ogg", &oggLen);
    if (!ogg) { for (;;) {} }
    DbgPrint("dsound-music: read music.ogg (%u bytes)\n", (unsigned)oggLen);

    static rxdk_stb_vorbis_alloc arena;
    arena.alloc_buffer = (char *)malloc(VORBIS_ARENA_BYTES);
    arena.alloc_buffer_length_in_bytes = VORBIS_ARENA_BYTES;
    if (!arena.alloc_buffer) { DbgPrint("dsound-music: arena malloc failed\n"); for (;;) {} }

    DbgPrint("dsound-music: opening vorbis with %u-byte arena at %p...\n",
             (unsigned)VORBIS_ARENA_BYTES, arena.alloc_buffer);
    g_vorbis = stb_vorbis_open_memory(ogg, (int)oggLen, &err, &arena);
    if (!g_vorbis) {
        DbgPrint("dsound-music: stb_vorbis_open_memory failed (err=%d)\n", err);
        for (;;) {}
    }
    info = stb_vorbis_get_info(g_vorbis);
    g_channels = info.channels;
    DbgPrint("dsound-music: %d ch, %u Hz\n", info.channels, info.sample_rate);

    // 2. DirectSound (the MCPX APU).
    DbgPrint("dsound-music: calling DirectSoundCreate...\n");
    hr = DirectSoundCreate(NULL, &pDS, NULL);
    DbgPrint("dsound-music: DirectSoundCreate returned hr=0x%08x pDS=%p\n", (unsigned)hr, (void *)pDS);
    if (FAILED(hr)) {
        DbgPrint("dsound-music: DirectSoundCreate failed hr=0x%08x\n", (unsigned)hr);
        for (;;) {}
    }

    // 3. Create a streaming sound (the APU pulls PCM from packets we submit).
    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = (WORD)info.channels;
    wfx.nSamplesPerSec  = info.sample_rate;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = (WORD)(info.channels * sizeof(short));
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    memset(&sd, 0, sizeof(sd));
    sd.dwMaxAttachedPackets = AUDIO_PACKETS;
    sd.lpwfxFormat          = &wfx;

    DbgPrint("dsound-music: calling CreateSoundStream...\n");
    hr = IDirectSound_CreateSoundStream(pDS, &sd, &stream, NULL);
    DbgPrint("dsound-music: CreateSoundStream returned hr=0x%08x stream=%p\n", (unsigned)hr, (void *)stream);
    if (FAILED(hr) || !stream) {
        DbgPrint("dsound-music: CreateSoundStream failed hr=0x%08x\n", (unsigned)hr);
        for (;;) {}
    }

    // Full volume, and route to the front-left/right speakers.
    IDirectSoundStream_SetVolume(stream, DSBVOLUME_MAX);
    IDirectSoundStream_SetHeadroom(stream, 0);

    mixPairs[0].dwMixBin = DSMIXBIN_FRONT_LEFT;    mixPairs[0].lVolume = DSBVOLUME_MAX;
    mixPairs[1].dwMixBin = DSMIXBIN_FRONT_RIGHT;   mixPairs[1].lVolume = DSBVOLUME_MAX;
    mixPairs[2].dwMixBin = DSMIXBIN_FRONT_CENTER;  mixPairs[2].lVolume = DSBVOLUME_MIN;
    mixPairs[3].dwMixBin = DSMIXBIN_LOW_FREQUENCY; mixPairs[3].lVolume = DSBVOLUME_MIN;
    mixPairs[4].dwMixBin = DSMIXBIN_BACK_LEFT;     mixPairs[4].lVolume = DSBVOLUME_MIN;
    mixPairs[5].dwMixBin = DSMIXBIN_BACK_RIGHT;    mixPairs[5].lVolume = DSBVOLUME_MIN;
    mixBins.dwMixBinCount      = 6;
    mixBins.lpMixBinVolumePairs = mixPairs;
    IDirectSoundStream_SetMixBins(stream, &mixBins);

    // 4. Per-packet decode buffer (cached) + status arrays.
    decodeBuffer  = (char *)malloc(AUDIO_PACKETS * AUDIO_OUTPUT_BUF_SIZE);
    packetStatus  = (DWORD *)malloc(AUDIO_PACKETS * sizeof(DWORD));
    completedSize = (DWORD *)malloc(AUDIO_PACKETS * sizeof(DWORD));
    if (!decodeBuffer || !packetStatus || !completedSize) {
        DbgPrint("dsound-music: packet buffer malloc failed\n"); for (;;) {}
    }
    memset(packetStatus, 0, AUDIO_PACKETS * sizeof(DWORD));   // 0 != PENDING -> all free
    memset(completedSize, 0, AUDIO_PACKETS * sizeof(DWORD));

    // 5. Stream forever: refill any packet the APU has finished with, looping the
    //    track on EOF. PCM stays in cached RAM; the APU DMAs from it.
    DbgPrint("dsound-music: streaming (looping)\n");
    for (;;) {
        DirectSoundDoWork();
        for (i = 0; i < AUDIO_PACKETS; i++) {
            if (packetStatus[i] == (DWORD)XMEDIAPACKET_STATUS_PENDING)
                continue;

            short *off       = (short *)(decodeBuffer + i * AUDIO_OUTPUT_BUF_SIZE);
            int    wantShorts = AUDIO_OUTPUT_BUF_SIZE / (int)sizeof(short);
            int    got        = stb_vorbis_get_samples_short_interleaved(g_vorbis, g_channels,
                                                                         off, wantShorts);
            int    bytes      = got * g_channels * (int)sizeof(short);

            if (bytes == 0) {            // end of stream -> loop
                stb_vorbis_seek_start(g_vorbis);
                continue;
            }
            if (bytes < AUDIO_OUTPUT_BUF_SIZE)
                memset((char *)off + bytes, 0, AUDIO_OUTPUT_BUF_SIZE - bytes);

            XMEDIAPACKET pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.pvBuffer         = off;
            pkt.dwMaxSize        = (DWORD)bytes;
            pkt.pdwCompletedSize = &completedSize[i];
            pkt.pdwStatus        = &packetStatus[i];
            IDirectSoundStream_Process(stream, &pkt, NULL);

            if (submitted < AUDIO_PACKETS && ++submitted == 1)
                DbgPrint("dsound-music: first packet submitted (%d bytes)\n", bytes);
        }
        Sleep(10);
    }

    return 0;
}
