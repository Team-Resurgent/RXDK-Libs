//------------------------------------------------------------------------------
// XMV playback -- RXDK libxmv sample.
//
// Decodes D:\media\test.xmv with the ported Xbox FMV decoder (libxmv: the leak
// private/windows/xmv software codec) into a YUY2 D3D image surface and shows it
// through the D3D8 hardware overlay. Double-buffered: GetNextFrame decodes into
// the "draw" surface, then we swap it to "show" and push it to the overlay.
// DirectSound (libdsound, the MCPX APU) is brought up and the title pumps it with
// DirectSoundDoWork, matching the retail XMV playback model.
//
// NOTE on audio: the leak decoder's audio path is not yet implemented
// (XMVDecoder_EnableAudioStream returns E_NOTIMPL), so the audio stream is
// gracefully left disabled and the video plays on its own. The DirectSound setup
// is kept (non-fatal) so audio lights up as soon as the decoder's audio path is
// ported (or a retail xmv.lib is linked).
//
// Assets are deployed alongside the XBE (D:\media\*) -- build-iso.ps1 xbcp's
// samples\xmv-play\media to xe:\DEVKIT\xbetest\xmv-play\media.
//------------------------------------------------------------------------------

// common.h first: pulls the xapi/xtl/NT environment (NT_INCLUDED + xboxkrnl base
// types) so the later d3d8/dsound headers use our types and skip MinGW <winnt.h>.
#include "common.h"   // xapi boot/trace helpers (shared with the xapi samples)
#include <stdio.h>
#include <string.h>
#include <guiddef.h>  // GUID/REFGUID -- d3d8.h's resource interfaces reference them

// COM calling-convention prelude that a full XDK supplies via <xtl.h>; the d3d8
// and dsound COM vtables need it (same set the lib builds provide via their
// site bridges -- see d3d8-triangle / dsound-music).
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
#include <d3d8.h>

// dsound.h's DECLARE_INTERFACE vtables need xobjbase.h's COM macros; its 3D-audio
// structs use D3DXVECTOR3 (we only need the bare type -- see dsound-music). dsound.h
// also supplies DSMIXBINS / IDirectSoundStream named by xmv.h's audio entry points.
#include <xobjbase.h>
#ifndef __D3DX8MATH_H__
typedef struct D3DXVECTOR3 { float x, y, z; } D3DXVECTOR3;
#endif
#include <dsound.h>
#include <dsstdfx.h>
#include <xmv.h>

#define XMV_PATH         "D:\\media\\test.xmv"
#define DSSTDFX_PATH     "D:\\media\\dsstdfx.bin"
#define SCREEN_W         640
#define SCREEN_H         480
#define PLAY_TIMEOUT_MS  120000
#define FRAME_SLEEP_MS   16
#define MIN_FRAMES       10

// libd3d8 / libdsound expose COM interfaces (IDirect3DDevice8, IDirectSound...)
// whose C++ vtables reference the Itanium-ABI pure-virtual handler. This C title
// links no libcpp, so supply the stub the C++ runtime would (same as dsound-music).
// Symbol: ___cxa_pure_virtual.
void __cxa_pure_virtual(void) { for (;;) {} }

static D3DDevice    *g_pd3dDevice = NULL;
static LPDIRECTSOUND g_pDSound    = NULL;

static HRESULT InitD3D(void)
{
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.BackBufferWidth        = SCREEN_W;
    d3dpp.BackBufferHeight       = SCREEN_H;
    d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount        = 1;
    d3dpp.Windowed               = FALSE;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    Direct3D_SetPushBufferSize(512 * 1024, (512 * 1024) / 16);

    hr = Direct3D_CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                               D3DCREATE_HARDWARE_VERTEXPROCESSING,
                               &d3dpp, &g_pd3dDevice);
    if (FAILED(hr)) {
        DbgPrint("xmv-play: CreateDevice failed hr=0x%08x\n", (unsigned)hr);
        return hr;
    }

    D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF202040, 1.0f, 0);
    D3DDevice_Swap(0);
    DbgPrint("xmv-play: Direct3D device created (%dx%d)\n", SCREEN_W, SCREEN_H);
    return S_OK;
}

// Bring up DirectSound (the MCPX APU) and download the standard effects image so
// the audio engine is ready. Returns S_OK on success; the caller treats failure
// as non-fatal (video still plays -- the decoder's XMV audio is NYI for now).
static HRESULT InitDirectSound(void)
{
    DSEFFECTIMAGELOC effectLoc;
    HRESULT hr;

    memset(&effectLoc, 0, sizeof(effectLoc));

    hr = DirectSoundCreate(NULL, &g_pDSound, NULL);
    if (FAILED(hr)) {
        DbgPrint("xmv-play: DirectSoundCreate failed hr=0x%08x\n", (unsigned)hr);
        return hr;
    }
    DbgPrint("xmv-play: DirectSound object created\n");

    effectLoc.dwI3DL2ReverbIndex = GraphI3DL2_I3DL2Reverb;
    effectLoc.dwCrosstalkIndex   = GraphXTalk_XTalk;

    // Prefer an embedded "dsstdfx" XBE section, fall back to the deployed file.
    hr = XAudioDownloadEffectsImage("dsstdfx", &effectLoc,
                                    XAUDIO_DOWNLOADFX_XBESECTION, NULL);
    if (FAILED(hr)) {
        hr = XAudioDownloadEffectsImage(DSSTDFX_PATH, &effectLoc,
                                        XAUDIO_DOWNLOADFX_EXTERNFILE, NULL);
    }
    if (FAILED(hr)) {
        DbgPrint("xmv-play: effects image not loaded hr=0x%08x "
                 "(embed dsstdfx.bin or deploy media\\dsstdfx.bin)\n", (unsigned)hr);
        return hr;
    }

    DbgPrint("xmv-play: DirectSound effects image loaded\n");
    return S_OK;
}

static HRESULT PlayXmv(void)
{
    XMVDecoder    *pDecoder    = NULL;
    XMVVIDEO_DESC  videoDesc;
    D3DSurface    *pSurfaceShow = NULL;
    D3DSurface    *pSurfaceDraw = NULL;
    D3DSurface    *pSurfaceSwap;
    RECT           sourceRect;
    RECT           destRect;
    XMVRESULT      xr;
    DWORD          frameCount = 0;
    DWORD          elapsedMs  = 0;
    BOOL           overlayEnabled = FALSE;
    BOOL           audioEnabled   = FALSE;
    HRESULT        hr;

    memset(&videoDesc, 0, sizeof(videoDesc));
    memset(&sourceRect, 0, sizeof(sourceRect));
    memset(&destRect, 0, sizeof(destRect));

    hr = InitD3D();
    if (FAILED(hr)) {
        return hr;
    }

    // DirectSound bring-up is non-fatal: XMV audio is NYI in the decoder, so a
    // missing effects image should not stop the video from playing.
    if (FAILED(InitDirectSound())) {
        DbgPrint("xmv-play: continuing without DirectSound (video only)\n");
    }

    DbgPrint("xmv-play: opening %s\n", XMV_PATH);
    hr = XMVDecoder_CreateDecoderForFile(XMVFLAG_NONE, XMV_PATH, &pDecoder);
    if (FAILED(hr)) {
        DbgPrint("xmv-play: open failed (hr=0x%08x) -- copy test.xmv to D:\\media\n",
                 (unsigned)hr);
        return hr;
    }
    DbgPrint("xmv-play: XMV decoder opened\n");

    XMVDecoder_GetVideoDescriptor(pDecoder, &videoDesc);
    DbgPrint("xmv-play: video %ux%u, %u fps, %u audio stream(s)\n",
             videoDesc.Width, videoDesc.Height,
             videoDesc.FramesPerSecond, videoDesc.AudioStreamCount);

    if (videoDesc.Width > 0 && videoDesc.Height > 0) {
        // Two YUY2 image surfaces, exactly the video size: GetNextFrame decodes
        // into the draw surface; on a new frame we swap and present the show one.
        hr = D3DDevice_CreateImageSurface(videoDesc.Width, videoDesc.Height,
                                          D3DFMT_YUY2, &pSurfaceShow);
        if (FAILED(hr)) {
            XMVDecoder_CloseDecoder(pDecoder);
            return hr;
        }
        hr = D3DDevice_CreateImageSurface(videoDesc.Width, videoDesc.Height,
                                          D3DFMT_YUY2, &pSurfaceDraw);
        if (FAILED(hr)) {
            XMVDecoder_CloseDecoder(pDecoder);
            return hr;
        }

        sourceRect.right  = (LONG)videoDesc.Width;
        sourceRect.bottom = (LONG)videoDesc.Height;
        destRect.right    = SCREEN_W;
        destRect.bottom   = SCREEN_H;
    }

    if (videoDesc.AudioStreamCount > 0) {
        hr = XMVDecoder_EnableAudioStream(pDecoder, 0, 0, NULL, NULL);
        if (FAILED(hr)) {
            DbgPrint("xmv-play: audio stream 0 disabled (hr=0x%08x)\n", (unsigned)hr);
        } else {
            audioEnabled = TRUE;
            DbgPrint("xmv-play: audio stream 0 enabled\n");
        }
    }

    DbgPrint("xmv-play: decoding frames...\n");

    for (;;) {
        hr = XMVDecoder_GetNextFrame(pDecoder, pSurfaceDraw, &xr, NULL);
        if (FAILED(hr)) {
            if (overlayEnabled) {
                D3DDevice_EnableOverlay(FALSE);
            }
            XMVDecoder_CloseDecoder(pDecoder);
            return hr;
        }

        switch (xr) {
        case XMV_NOFRAME:
            break;

        case XMV_NEWFRAME:
            ++frameCount;
            if (pSurfaceDraw && pSurfaceShow) {
                pSurfaceSwap = pSurfaceShow;
                pSurfaceShow = pSurfaceDraw;
                pSurfaceDraw = pSurfaceSwap;

                if (!overlayEnabled) {
                    D3DDevice_EnableOverlay(TRUE);
                    overlayEnabled = TRUE;
                }
                while (!D3DDevice_GetOverlayUpdateStatus()) {
                    ;
                }
                D3DDevice_UpdateOverlay(pSurfaceShow, &sourceRect, &destRect, FALSE, 0);
            }
            break;

        case XMV_ENDOFFILE:
            if (overlayEnabled) {
                D3DDevice_EnableOverlay(FALSE);
            }
            DbgPrint("xmv-play: decoded %u frames in ~%u ms\n", frameCount, elapsedMs);
            XMVDecoder_CloseDecoder(pDecoder);
            if (videoDesc.Width > 0 && frameCount < MIN_FRAMES) {
                DbgPrint("xmv-play: too few frames (%u) -- check test.xmv\n", frameCount);
                return E_FAIL;
            }
            DbgPrint("xmv-play: *** playback finished ***\n");
            return S_OK;

        case XMV_FAIL:
        default:
            if (overlayEnabled) {
                D3DDevice_EnableOverlay(FALSE);
            }
            DbgPrint("xmv-play: decode error after %u frames\n", frameCount);
            XMVDecoder_CloseDecoder(pDecoder);
            return E_FAIL;
        }

        if (audioEnabled || g_pDSound) {
            DirectSoundDoWork();
        }
        D3DDevice_BlockUntilVerticalBlank();
        elapsedMs += FRAME_SLEEP_MS;
        if (elapsedMs >= PLAY_TIMEOUT_MS) {
            if (overlayEnabled) {
                D3DDevice_EnableOverlay(FALSE);
            }
            DbgPrint("xmv-play: timed out after %u ms (%u frames)\n", elapsedMs, frameCount);
            XMVDecoder_CloseDecoder(pDecoder);
            return E_FAIL;
        }
    }
}

static void HangForever(void)
{
    for (;;) {
        if (g_pDSound) {
            DirectSoundDoWork();
        }
        if (g_pd3dDevice) {
            D3DDevice_BlockUntilVerticalBlank();
        } else {
            Sleep(FRAME_SLEEP_MS);
        }
    }
}

int main(void)
{
    HRESULT hr;

    xapi_smoke_trace_line("xmv-play start");
    DbgPrint("xmv-play: starting XMV playback sample (libxmv)\n");

    hr = PlayXmv();
    if (FAILED(hr)) {
        DbgPrint("xmv-play: playback failed (hr=0x%08x)\n", (unsigned)hr);
        HangForever();
        return 1;
    }

    DbgPrint("xmv-play: done\n");
    HangForever();
    return 0;
}
