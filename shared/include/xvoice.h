/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Low-level Xbox voice (communicator) API. Declares the IXVoiceEncoder and
 * IXVoiceDecoder XMediaObjects that compress mic PCM to voice packets and back,
 * the communicator-headset capture/render media objects, the queueing decoders
 * for buffered playback, voice-mask (disguise) parameters, and packet-energy
 * measurement. Mutually exclusive with the high-level XHV API (<xhv.h>).
 */

#ifndef __XVOICE__
#define __XVOICE__

#ifdef __XHV__
#error High-level(XHV) and low-level voice APIs cannot be used in the same time!
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Voice masks disguise a talker by reshaping the encoded voice. Each field is a
// normalized [0,1] weight; XVOICE_MASK_PARAM_DISABLED (-1.0) leaves that effect
// off. The XVOICE_MASK_* presets are ready-made XVOICE_MASK initializers.
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)

// Predefined voice masks          fSpecEnergyWeight           fPitchScale                 fWhisperValue               fRoboticValue
#define XVOICE_MASK_NONE { XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_ANONYMOUS { 0.95f, 0.50f, 0.35f, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_CARTOON { 0.10f, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_BIGGUY { 0.90f, 0.05f, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_CHILD { 0.10f, 0.70f, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_ROBOT { 0.50f, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 0.05f }
#define XVOICE_MASK_DARKMASTER { 1.00f, 0.00f, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED }
#define XVOICE_MASK_WHISPER { 0.50f, XVOICE_MASK_PARAM_DISABLED, 0.01f, XVOICE_MASK_PARAM_DISABLED }

#pragma pack(push, 4)

typedef struct _XVOICE_MASK {
    FLOAT fSpecEnergyWeight;
    FLOAT fPitchScale;
    FLOAT fWhisperValue;
    FLOAT fRoboticValue;

} XVOICE_MASK, *LPXVOICE_MASK;

typedef struct _XVOICE_VOICE_PROPERTIES {
    FLOAT fGain;
    FLOAT fPrediction[12];
    FLOAT fReflection[12];
    LARGE_INTEGER liTimestamp;

} XVOICE_VOICE_PROPERTIES, *LPXVOICE_VOICE_PROPERTIES;

// Configuration for the queueing voice media objects: packet duration (ms),
// codec buffer size, the min/max jitter-buffer delay bounds, the initial
// high-water mark, and a target quality used to trade latency against dropouts.
typedef struct _XVOICE_QUEUE_XMO_CONFIG {
    DWORD dwMsOfDataPerPacket;
    DWORD dwCodecBufferSize;
    DWORD dwMaxDelay;
    DWORD dwMinDelay;
    DWORD dwInitialHighWaterMark;
    DOUBLE dIdealQuality;

} XVOICE_QUEUE_XMO_CONFIG, *LPXVOICE_QUEUE_XMO_CONFIG;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct _XVOICE_CODEC_HEADER {
    WORD bMsgNo : 4;
    WORD wSeqNo : 12;

} XVOICE_CODEC_HEADER, *LPXVOICE_CODEC_HEADER;

#pragma pack(pop)

#define XVOICE_MAX_SEQ (0x0FFF + 1)
#define XVOICE_MAX_STREAMS 32

typedef DWORD XVOICE_STREAM_INDEX, *LPXVOICE_STREAM_INDEX;

// Voice encoder: an XMediaObject that compresses microphone PCM into voice
// packets. ProcessMultiple encodes several talker streams at once; SetVoiceMask
// applies a disguise to one encoder instance; GetCodecBufferSize reports the
// compressed size for a given PCM buffer. (Single-packet Process is unused.)
#undef INTERFACE
#define INTERFACE IXVoiceEncoder

DECLARE_INTERFACE_(IXVoiceEncoder, XMediaObject)
{
    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // XMediaObject methods
    STDMETHOD(GetInfo)(THIS_ LPXMEDIAINFO pInfo) PURE;
    STDMETHOD(GetStatus)(THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Process)(THIS_ LPCXMEDIAPACKET pInputPacket, LPCXMEDIAPACKET pOutputPacket) PURE; // returns "not implemented"
    STDMETHOD(Discontinuity)(THIS) PURE;
    STDMETHOD(Flush)(THIS) PURE;

    // IXVoiceEncoder methods
    STDMETHOD(ProcessMultiple)(THIS_ DWORD dwInputArraySize, LPCXMEDIAPACKET pInputPacketArray,
        DWORD dwOutputArraySize, LPCXMEDIAPACKET pOutputPacketArray) PURE;
    STDMETHOD(SetVoiceMask)(THIS_ DWORD dwEncoderInstanceNo, LPXVOICE_MASK pVoiceMask) PURE;
    STDMETHOD(GetCodecBufferSize)(THIS_ DWORD dwPCMBufferSize, DWORD * pdwCodecBufferSize) PURE;
};

#define IXVoiceEncoder_AddRef IUnknown_AddRef
#define IXVoiceEncoder_Release IUnknown_Release

#define IXVoiceEncoder_GetInfo XMediaObject_GetInfo
#define IXVoiceEncoder_GetStatus XMediaObject_GetStatus
#define IXVoiceEncoder_Process XMediaObject_Process
#define IXVoiceEncoder_Discontinuity XMediaObject_Discontinuity
#define IXVoiceEncoder_Flush XMediaObject_Flush

#if defined(__cplusplus) && !defined(CINTERFACE)

#define IXVoiceEncoder_ProcessMultiple(p, a, b, c, d) p->ProcessMultiple(a, b, c, d)
#define IXVoiceEncoder_SetVoiceMask(p, a, b) p->SetVoiceMask(a, b)
#define IXVoiceEncoder_GetCodecBufferSize(p, a, b) p->GetCodecBufferSize(a, b)

#else // defined(__cplusplus) && !defined(CINTERFACE)

#define IXVoiceEncoder_ProcessMultiple(p, a, b, c, d) p->lpVtbl->ProcessMultiple(p, a, b, c, d)
#define IXVoiceEncoder_SetVoiceMask(p, a, b) p->lpVtbl->SetVoiceMask(p, a, b)
#define IXVoiceEncoder_GetCodecBufferSize(p, a, b) p->lpVtbl->GetCodecBufferSize(p, a, b)

#endif // defined(__cplusplus) && !defined(CINTERFACE)

typedef struct IXVoiceEncoder IXVoiceEncoder, *LPXVOICEENCODER;


// Voice decoder: an XMediaObject that expands received voice packets back to
// PCM. ProcessMultiple decodes several streams at once; GetVoiceProperties,
// called right after, returns per-stream gain/LPC data for the frames just
// decoded (e.g. to drive talker indicators).
#undef INTERFACE
#define INTERFACE IXVoiceDecoder

DECLARE_INTERFACE_(IXVoiceDecoder, XMediaObject)
{
    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // XMediaObject methods
    STDMETHOD(GetInfo)(THIS_ LPXMEDIAINFO pInfo) PURE;
    STDMETHOD(GetStatus)(THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Process)(THIS_ LPCXMEDIAPACKET pInputPacket, LPCXMEDIAPACKET pOutputPacket) PURE; // returns "not implemented"
    STDMETHOD(Discontinuity)(THIS) PURE;
    STDMETHOD(Flush)(THIS) PURE;

    // IXVoiceDecoder methods
    STDMETHOD(ProcessMultiple)(THIS_ DWORD dwInputArraySize, LPCXMEDIAPACKET pInputPacketArray,
        DWORD dwOutputArraySize, LPCXMEDIAPACKET pOutputPacketArray) PURE;
    // Gets called immediately after ProcessMultiple to retrieve the voice properties for the last decoded frames
    STDMETHOD(GetVoiceProperties)(THIS_ DWORD dwArraySize, LPXVOICE_VOICE_PROPERTIES pPropertiesArray) PURE;
};

#define IXVoiceDecoder_AddRef IUnknown_AddRef
#define IXVoiceDecoder_Release IUnknown_Release

#define IXVoiceDecoder_GetInfo XMediaObject_GetInfo
#define IXVoiceDecoder_GetStatus XMediaObject_GetStatus
#define IXVoiceDecoder_Process XMediaObject_Process
#define IXVoiceDecoder_Discontinuity XMediaObject_Discontinuity
#define IXVoiceDecoder_Flush XMediaObject_Flush

#if defined(__cplusplus) && !defined(CINTERFACE)

#define IXVoiceDecoder_ProcessMultiple(p, a, b, c, d) p->ProcessMultiple(a, b, c, d)
#define IXVoiceDecoder_GetVoiceProperties(p, a, b) p->GetVoiceProperties(a, b)

#else // defined(__cplusplus) && !defined(CINTERFACE)

#define IXVoiceDecoder_ProcessMultiple(p, a, b, c, d) p->lpVtbl->ProcessMultiple(p, a, b, c, d)
#define IXVoiceDecoder_GetVoiceProperties(p, a, b) p->lpVtbl->GetVoiceProperties(p, a, b)

#endif // defined(__cplusplus) && !defined(CINTERFACE)

typedef struct IXVoiceDecoder IXVoiceDecoder, *LPXVOICEDECODER;

// Factories. XVoiceCreateMediaObject[Ex] opens the communicator headset on a
// controller port as a capture/render XMediaObject. The Queue variants add jitter
// buffering. The Create*Encoder / Create*Decoder helpers build encoder/decoder
// objects for a given talker topology (one-to-one, two-to-two, four-to-four, and
// the mixing decoders). XVoiceGetEncodedPacketEnergy returns a packet's RMS
// energy for voice-activity display.
XBOXAPI
HRESULT
WINAPI
XVoiceCreateMediaObject(
    IN PXPP_DEVICE_TYPE XppDeviceType,
    IN DWORD dwPort,
    IN DWORD dwMaxAttachedPackets,
    IN LPCWAVEFORMATEX pwfx,
    OUT LPXMEDIAOBJECT* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateMediaObjectEx(
    IN PXPP_DEVICE_TYPE XppDeviceType,
    IN DWORD dwPort,
    IN DWORD dwMaxAttachedPackets,
    IN LPCWAVEFORMATEX pwfx,
    IN PFNXMEDIAOBJECTCALLBACK pfnCallback,
    IN PVOID pvContext,
    OUT LPXMEDIAOBJECT* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceQueueCreateMediaObject(
    IN LPXVOICE_QUEUE_XMO_CONFIG pConfig,
    OUT LPXMEDIAOBJECT* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceSynchronizedQueueCreateMediaObject(
    IN LPXVOICE_QUEUE_XMO_CONFIG pConfig,
    OUT LPXMEDIAOBJECT* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateOneToOneEncoder(
    OUT LPXVOICEENCODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateTwoToTwoEncoder(
    OUT LPXVOICEENCODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateFourToFourFullEncoder(
    OUT LPXVOICEENCODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateFourToFourRoundRobinEncoder(
    OUT LPXVOICEENCODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateOneToOneDecoder(
    OUT LPXVOICEDECODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateFourToFourFullDecoder(
    IN DWORD dwMaxStreams,
    OUT LPXVOICEDECODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateFourToOneMixingDecoder(
    IN DWORD dwMaxStreams,
    OUT LPXVOICEDECODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceCreateTwoToOneMixingDecoder(
    IN DWORD dwMaxStreams,
    OUT LPXVOICEDECODER* ppMediaObject);

XBOXAPI
HRESULT
WINAPI
XVoiceGetEncodedPacketEnergy(
    IN LPCXMEDIAPACKET pPacket,
    OUT FLOAT* pfEnergy);

#ifdef __cplusplus
}
#endif

#endif // __XVOICE__
