// RXDK 5849 uplift: the low-level voice API (xvoice.h / xvoice.lib) + the
// XDEVICE_TYPE_VOICE_* device tables.
//
// The leak carries no xvoice implementation at all. The low-level surface is
// two things RXDK cannot make real:
//  - XVoiceCreateMediaObject[Ex]: an XMO over the voice communicator's USB
//    microphone/headphone endpoints. The USB audio class driver was never in
//    the leak, so no communicator device can exist; the retail lib fails this
//    call when no communicator is inserted at the given port, and that is the
//    failure we reproduce.
//  - the SC03 voice encoder/decoder XMO factories: the codec exists only as
//    binaries inside the retail lib. Absent a codec, the factories fail
//    cleanly (E_FAIL, *ppMediaObject = NULL) so titles degrade the same way
//    they would on codec-init failure.
// The queue XMOs sit between the two (pure software jitter buffers), but with
// neither capture nor codec they can never be fed, so they fail alike rather
// than hand the title a working object whose neighbors are absent.
// TODO(5849-xvoice): real support needs the USB audio driver + a codec.

#include <xvoice.h>

extern "C" {

// The voice communicator XPP device tables (exported by the retail xvoice.lib,
// declared in Xbox.h). Zero-initialized like every DECLARE_XPP_TYPE table in
// libxapi's USB stack; with no voice USB class driver to register devices they
// stay empty, so XGetDevices() reports no communicators on any port.
XPP_DEVICE_TYPE XDEVICE_TYPE_VOICE_MICROPHONE_TABLE = { { 0, 0, 0 } };
XPP_DEVICE_TYPE XDEVICE_TYPE_VOICE_HEADPHONE_TABLE  = { { 0, 0, 0 } };

XBOXAPI HRESULT WINAPI XVoiceCreateMediaObject(PXPP_DEVICE_TYPE XppDeviceType, DWORD dwPort,
                                               DWORD dwMaxAttachedPackets, LPCWAVEFORMATEX pwfx,
                                               LPXMEDIAOBJECT *ppMediaObject)
{
    (void)XppDeviceType;
    (void)dwPort;
    (void)dwMaxAttachedPackets;
    (void)pwfx;
    if (!ppMediaObject)
        return E_POINTER;
    *ppMediaObject = NULL;
    return E_FAIL; // no communicator device can be present (no USB audio driver)
}

XBOXAPI HRESULT WINAPI XVoiceCreateMediaObjectEx(PXPP_DEVICE_TYPE XppDeviceType, DWORD dwPort,
                                                 DWORD dwMaxAttachedPackets, LPCWAVEFORMATEX pwfx,
                                                 PFNXMEDIAOBJECTCALLBACK pfnCallback, PVOID pvContext,
                                                 LPXMEDIAOBJECT *ppMediaObject)
{
    (void)pfnCallback;
    (void)pvContext;
    return XVoiceCreateMediaObject(XppDeviceType, dwPort, dwMaxAttachedPackets, pwfx, ppMediaObject);
}

static HRESULT NullOut(LPXMEDIAOBJECT *ppMediaObject)
{
    if (!ppMediaObject)
        return E_POINTER;
    *ppMediaObject = NULL;
    return E_FAIL;
}

XBOXAPI HRESULT WINAPI XVoiceQueueCreateMediaObject(LPXVOICE_QUEUE_XMO_CONFIG pConfig,
                                                    LPXMEDIAOBJECT *ppMediaObject)
{
    (void)pConfig;
    return NullOut(ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceSynchronizedQueueCreateMediaObject(LPXVOICE_QUEUE_XMO_CONFIG pConfig,
                                                                LPXMEDIAOBJECT *ppMediaObject)
{
    (void)pConfig;
    return NullOut(ppMediaObject);
}

// SC03 encoder factories.
XBOXAPI HRESULT WINAPI XVoiceCreateOneToOneEncoder(LPXVOICEENCODER *ppMediaObject)
{
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateTwoToTwoEncoder(LPXVOICEENCODER *ppMediaObject)
{
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateFourToFourFullEncoder(LPXVOICEENCODER *ppMediaObject)
{
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateFourToFourRoundRobinEncoder(LPXVOICEENCODER *ppMediaObject)
{
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

// SC03 decoder factories.
XBOXAPI HRESULT WINAPI XVoiceCreateOneToOneDecoder(LPXVOICEDECODER *ppMediaObject)
{
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateFourToFourFullDecoder(DWORD dwMaxStreams,
                                                         LPXVOICEDECODER *ppMediaObject)
{
    (void)dwMaxStreams;
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateFourToOneMixingDecoder(DWORD dwMaxStreams,
                                                          LPXVOICEDECODER *ppMediaObject)
{
    (void)dwMaxStreams;
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceCreateTwoToOneMixingDecoder(DWORD dwMaxStreams,
                                                         LPXVOICEDECODER *ppMediaObject)
{
    (void)dwMaxStreams;
    return NullOut((LPXMEDIAOBJECT *)ppMediaObject);
}

XBOXAPI HRESULT WINAPI XVoiceGetEncodedPacketEnergy(LPCXMEDIAPACKET pPacket, FLOAT *pfEnergy)
{
    (void)pPacket;
    if (!pfEnergy)
        return E_POINTER;
    *pfEnergy = 0.0f;
    return E_FAIL; // no encoder exists to have produced a packet
}

} // extern "C"
