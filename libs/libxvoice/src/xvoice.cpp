/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// The low-level voice API (xvoice.h / xvoice.lib): SC03 codec + queue XMOs.
//
// The communicator itself is REAL now: XVoiceCreateMediaObject[Ex] and the
// XDEVICE_TYPE_VOICE_* device tables live in the Hawk USB class driver ported
// into libxapi (libs/libxapi/usb/hawk/hawk2.cpp — retail ships that driver in
// xvoice.lib, RXDK keeps it with the other class drivers because the class
// list is a static table in usbd.cpp). Raw PCM capture/playback through a
// communicator works end to end.
//
// What remains stubbed here is the SC03 voice codec and the queue XMOs:
//  - the SC03 encoder/decoder ships only as binaries in the retail lib.
//    Absent a codec, the factories fail cleanly (E_FAIL, *ppMediaObject =
//    NULL) so titles degrade the same way they would on codec-init failure.
//  - the queue XMOs are pure software jitter buffers between capture and
//    codec; without the codec they fail alike rather than hand the title a
//    working object whose neighbors are absent.
//
// The codec stubs are a CAPABILITY BOUNDARY, not unfinished work (see the
// note at the top of xhv.cpp): SC03 ships only as binaries in the retail libs.

#include <xvoice.h>

extern "C" {

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
