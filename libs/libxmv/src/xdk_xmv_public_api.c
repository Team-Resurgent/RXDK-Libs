/* Public stdcall exports (out/include/xmv.h) wrapping the internal decoder API. */

#include <xtl.h>
#include <xmv.h>

typedef struct _XMVVideoDescriptor {
    DWORD Width;
    DWORD Height;
    DWORD FramesPerSecond;
    DWORD AudioStreamCount;
} XMVVideoDescriptor;

HRESULT XMVCreateDecoder(char *szFileName, XMVDecoder **ppDecoder);
void XMVCloseDecoder(XMVDecoder *pDecoder);
void XMVGetVideoDescriptor(XMVDecoder *pDecoder, XMVVideoDescriptor *pVideoDescriptor);
HRESULT XMVEnableAudioStream(
    XMVDecoder *pDecoder,
    DWORD AudioStream,
    DWORD Flags,
    DSMIXBINS *pMixBins,
    IDirectSoundStream **ppStream);
XMVRESULT XMVGetNextFrame(XMVDecoder *pDecoder, D3DSurface *pSurface);

HRESULT __stdcall XMVDecoder_CreateDecoderForFile(
    DWORD Flags,
    LPCSTR szFileName,
    XMVDecoder **ppDecoder)
{
    (void)Flags;
    if (!szFileName || !ppDecoder) {
        return E_INVALIDARG;
    }
    return XMVCreateDecoder((char *)szFileName, ppDecoder);
}

void __stdcall XMVDecoder_CloseDecoder(XMVDecoder *pDecoder)
{
    XMVCloseDecoder(pDecoder);
}

void __stdcall XMVDecoder_GetVideoDescriptor(
    XMVDecoder *pDecoder,
    XMVVIDEO_DESC *pVideoDescriptor)
{
    if (!pDecoder || !pVideoDescriptor) {
        return;
    }
    XMVGetVideoDescriptor(pDecoder, (XMVVideoDescriptor *)pVideoDescriptor);
}

HRESULT __stdcall XMVDecoder_EnableAudioStream(
    XMVDecoder *pDecoder,
    DWORD AudioStream,
    DWORD Flags,
    DSMIXBINS *pMixBins,
    IDirectSoundStream **ppStream)
{
    return XMVEnableAudioStream(pDecoder, AudioStream, Flags, pMixBins, ppStream);
}

HRESULT __stdcall XMVDecoder_GetNextFrame(
    XMVDecoder *pDecoder,
    IDirect3DSurface8 *pSurface,
    XMVRESULT *pResult,
    DWORD *pTimeOfFrame)
{
    XMVRESULT xr;

    if (!pDecoder) {
        return E_INVALIDARG;
    }

    xr = XMVGetNextFrame(pDecoder, (D3DSurface *)pSurface);
    if (pResult) {
        *pResult = xr;
    }
    if (pTimeOfFrame) {
        *pTimeOfFrame = 0;
    }
    return S_OK;
}
