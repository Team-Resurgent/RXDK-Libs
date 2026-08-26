/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Minimal ASF (Advanced Systems Format) reader. Field layouts follow the ASF
 * specification; everything is little-endian on disk. Pure C, built in the
 * minimal (picolibc) WMA slice.
 */

#include "asf.h"
#include <string.h>

// ---------------------------------------------------------------------------------------------
// Little-endian readers
// ---------------------------------------------------------------------------------------------

static unsigned short rd16(const unsigned char *p)
{
    return (unsigned short)(p[0] | ((unsigned)p[1] << 8));
}

static unsigned int rd32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

// ASF stores 64-bit quantities, but everything we read from them (file size, durations, packet
// counts) fits comfortably in 32 bits for any real .wma, and the rest of the audio stack is
// 32-bit anyway. Saturate rather than wrap so a bogus high word can't produce a small number.
static unsigned int rd64sat(const unsigned char *p)
{
    unsigned int lo = rd32(p);
    unsigned int hi = rd32(p + 4);
    return hi ? 0xFFFFFFFFu : lo;
}

// ---------------------------------------------------------------------------------------------
// Object GUIDs (stored as the usual mixed-endian GUID: DWORD, WORD, WORD, then 8 raw bytes)
// ---------------------------------------------------------------------------------------------

static const unsigned char GUID_HEADER[16] =
    { 0x30,0x26,0xB2,0x75, 0x8E,0x66, 0xCF,0x11, 0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C };
static const unsigned char GUID_FILE_PROPERTIES[16] =
    { 0xA1,0xDC,0xAB,0x8C, 0x47,0xA9, 0xCF,0x11, 0x8E,0xE4,0x00,0xC0,0x0C,0x20,0x53,0x65 };
static const unsigned char GUID_STREAM_PROPERTIES[16] =
    { 0x91,0x07,0xDC,0xB7, 0xB7,0xA9, 0xCF,0x11, 0x8E,0xE6,0x00,0xC0,0x0C,0x20,0x53,0x65 };
static const unsigned char GUID_CONTENT_DESCRIPTION[16] =
    { 0x33,0x26,0xB2,0x75, 0x8E,0x66, 0xCF,0x11, 0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C };
static const unsigned char GUID_DATA[16] =
    { 0x36,0x26,0xB2,0x75, 0x8E,0x66, 0xCF,0x11, 0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C };
static const unsigned char GUID_HEADER_EXTENSION[16] =
    { 0xB5,0x03,0xBF,0x5F, 0x2E,0xA9, 0xCF,0x11, 0x8E,0xE3,0x00,0xC0,0x0C,0x20,0x53,0x65 };
static const unsigned char GUID_AUDIO_MEDIA[16] =
    { 0x40,0x9E,0x69,0xF8, 0x4D,0x5B, 0xCF,0x11, 0xA8,0xFD,0x00,0x80,0x5F,0x5C,0x44,0x2B };

static int guid_eq(const unsigned char *a, const unsigned char *b)
{
    return 0 == memcmp(a, b, 16);
}

// ---------------------------------------------------------------------------------------------
// Header parsing
// ---------------------------------------------------------------------------------------------

// ASF strings are UTF-16LE and include their terminating NUL in the byte count.
static void copy_string(unsigned short *pDst, const unsigned char *pSrc, unsigned int cbSrc)
{
    unsigned int chars = cbSrc / 2;
    unsigned int i;

    if (chars > ASF_MAX_STRING_CHARS - 1)
        chars = ASF_MAX_STRING_CHARS - 1;

    for (i = 0; i < chars; i++)
        pDst[i] = rd16(pSrc + i * 2);
    pDst[chars] = 0;
}

int AsfPeekHeaderSize(const unsigned char *pPrefix, unsigned int cbPrefix, unsigned int *pcbHeader)
{
    if (!pPrefix || !pcbHeader || cbPrefix < 30)
        return -1;
    if (!guid_eq(pPrefix, GUID_HEADER))
        return -1;

    *pcbHeader = rd64sat(pPrefix + 16);
    if (*pcbHeader < 30)
        return -1;

    return 0;
}

static int parse_stream_properties(const unsigned char *p, unsigned int cb, AsfFileInfo *pInfo)
{
    unsigned int cbTypeSpecific;
    const unsigned char *pwfx;
    unsigned int cbExtra;

    // StreamType(16) ErrorCorrectionType(16) TimeOffset(8) TypeSpecificLen(4)
    // ErrorCorrectionLen(4) Flags(2) Reserved(4)
    if (cb < 54)
        return -1;
    if (!guid_eq(p, GUID_AUDIO_MEDIA))
        return 1;                       // not the audio stream -- skip it, not an error

    cbTypeSpecific = rd32(p + 40);
    if (cbTypeSpecific < 18 || 54 + cbTypeSpecific > cb)
        return -1;

    pwfx = p + 54;
    pInfo->formatTag      = rd16(pwfx + 0);
    pInfo->channels       = rd16(pwfx + 2);
    pInfo->sampleRate     = rd32(pwfx + 4);
    pInfo->avgBytesPerSec = rd32(pwfx + 8);
    pInfo->blockAlign     = rd16(pwfx + 12);
    pInfo->bitsPerSample  = rd16(pwfx + 14);

    cbExtra = rd16(pwfx + 16);
    if (cbExtra > cbTypeSpecific - 18)
        cbExtra = cbTypeSpecific - 18;
    if (cbExtra > sizeof(pInfo->extradata))
        cbExtra = sizeof(pInfo->extradata);

    memcpy(pInfo->extradata, pwfx + 18, cbExtra);
    pInfo->extradataSize = (unsigned short)cbExtra;

    return 0;
}

static void parse_file_properties(const unsigned char *p, unsigned int cb, AsfFileInfo *pInfo)
{
    unsigned int playDuration100ns;
    unsigned int durationMs;

    // FileID(16) FileSize(8) CreationDate(8) DataPacketsCount(8) PlayDuration(8) SendDuration(8)
    // Preroll(8) Flags(4) MinPacketSize(4) MaxPacketSize(4) MaxBitrate(4)
    if (cb < 80)
        return;

    pInfo->dataPacketCount = rd64sat(p + 32);
    playDuration100ns      = rd64sat(p + 40);
    pInfo->preroll         = rd64sat(p + 56);
    pInfo->packetSize      = rd32(p + 68);      // MinPacketSize; == MaxPacketSize for WMA

    // Play duration is in 100ns units and includes the preroll pad the encoder prepended.
    durationMs = (playDuration100ns == 0xFFFFFFFFu) ? 0xFFFFFFFFu : playDuration100ns / 10000u;
    pInfo->durationMs = (durationMs > pInfo->preroll) ? durationMs - pInfo->preroll : 0;
}

static void parse_content_description(const unsigned char *p, unsigned int cb, AsfFileInfo *pInfo)
{
    unsigned int lens[5];
    unsigned int total = 0;
    unsigned int off;
    int i;

    if (cb < 10)
        return;

    for (i = 0; i < 5; i++)
    {
        lens[i] = rd16(p + i * 2);
        total += lens[i];
    }
    if (10 + total > cb)
        return;

    off = 10;
    copy_string(pInfo->title,       p + off, lens[0]); off += lens[0];
    copy_string(pInfo->author,      p + off, lens[1]); off += lens[1];
    copy_string(pInfo->copyright,   p + off, lens[2]); off += lens[2];
    copy_string(pInfo->description, p + off, lens[3]); off += lens[3];
    copy_string(pInfo->rating,      p + off, lens[4]);
}

int AsfParseHeader(const unsigned char *pHeader, unsigned int cbHeader, AsfFileInfo *pInfo)
{
    unsigned int cbTotal;
    unsigned int off;
    int haveAudio = 0;

    if (!pHeader || !pInfo)
        return -1;
    memset(pInfo, 0, sizeof(*pInfo));

    if (AsfPeekHeaderSize(pHeader, cbHeader, &cbTotal) < 0)
        return -1;
    if (cbTotal > cbHeader)
        return -1;

    // Header Object: GUID(16) Size(8) NumHeaderObjects(4) Reserved(2)
    off = 30;

    while (off + 24 <= cbTotal)
    {
        const unsigned char *pObj = pHeader + off;
        unsigned int cbObj = rd64sat(pObj + 16);
        unsigned int cbBody;

        if (cbObj < 24 || off + cbObj > cbTotal)
            break;

        cbBody = cbObj - 24;

        if (guid_eq(pObj, GUID_FILE_PROPERTIES))
        {
            parse_file_properties(pObj + 24, cbBody, pInfo);
        }
        else if (guid_eq(pObj, GUID_STREAM_PROPERTIES))
        {
            if (!haveAudio && parse_stream_properties(pObj + 24, cbBody, pInfo) == 0)
                haveAudio = 1;
        }
        else if (guid_eq(pObj, GUID_CONTENT_DESCRIPTION))
        {
            parse_content_description(pObj + 24, cbBody, pInfo);
        }
        else if (guid_eq(pObj, GUID_HEADER_EXTENSION))
        {
            // Extended Stream Properties can live in here, but the plain Stream Properties object
            // is mandatory too, so there is nothing we need from it.
        }

        off += cbObj;
    }

    if (!haveAudio || pInfo->packetSize == 0 || pInfo->blockAlign == 0)
        return -1;

    // Data Object: GUID(16) Size(8) FileID(16) TotalDataPackets(8) Reserved(2) = 50 bytes,
    // then the packets themselves.
    if (cbTotal + 50 > cbHeader || !guid_eq(pHeader + cbTotal, GUID_DATA))
        return -1;

    pInfo->dataOffset = cbTotal + 50;

    return 0;
}

// ---------------------------------------------------------------------------------------------
// Packet parsing
// ---------------------------------------------------------------------------------------------

// The ASF packet header encodes several lengths with a 2-bit type: 0 = absent, 1 = BYTE,
// 2 = WORD, 3 = DWORD.
static unsigned int read_varlen(const unsigned char *p, unsigned int type, unsigned int *pOff)
{
    unsigned int v = 0;

    switch (type)
    {
        case 1: v = p[*pOff];        *pOff += 1; break;
        case 2: v = rd16(p + *pOff); *pOff += 2; break;
        case 3: v = rd32(p + *pOff); *pOff += 4; break;
        default: break;
    }

    return v;
}

static unsigned int varlen_size(unsigned int type)
{
    return (type == 1) ? 1u : (type == 2) ? 2u : (type == 3) ? 4u : 0u;
}

int AsfParsePacket(const AsfFileInfo *pInfo,
                   const unsigned char *pPacket, unsigned int cbPacket,
                   unsigned char *pOut, unsigned int cbOut, unsigned int *pcbOut)
{
    unsigned int off = 0;
    unsigned int lengthTypeFlags;
    unsigned int propertyFlags;
    unsigned int packetLength;
    unsigned int paddingLength;
    unsigned int payloadCount;
    unsigned int payloadLengthType = 0;
    unsigned int written = 0;
    unsigned int dataEnd;
    unsigned int i;

    if (!pInfo || !pPacket || !pOut || !pcbOut)
        return -1;
    *pcbOut = 0;
    if (cbPacket < 2)
        return -1;

    // Optional error-correction data comes first. Its presence is signalled by the top bit of the
    // first byte; the low nibble is then its length.
    if (pPacket[0] & 0x80)
    {
        unsigned int ecLen = pPacket[0] & 0x0F;
        off += 1 + ecLen;
        if (off + 2 > cbPacket)
            return -1;
    }

    lengthTypeFlags = pPacket[off++];
    propertyFlags   = pPacket[off++];

    {
        unsigned int packetLengthType  = (lengthTypeFlags >> 5) & 0x03;
        unsigned int paddingLengthType = (lengthTypeFlags >> 3) & 0x03;
        unsigned int sequenceType      = (lengthTypeFlags >> 1) & 0x03;
        unsigned int multiplePayloads  = lengthTypeFlags & 0x01;

        if (off + varlen_size(packetLengthType) + varlen_size(sequenceType) +
                   varlen_size(paddingLengthType) + 6 > cbPacket)
            return -1;

        packetLength = read_varlen(pPacket, packetLengthType, &off);
        (void)read_varlen(pPacket, sequenceType, &off);         // sequence -- unused for audio
        paddingLength = read_varlen(pPacket, paddingLengthType, &off);

        off += 4;   // Send Time
        off += 2;   // Duration

        if (packetLengthType == 0)
            packetLength = cbPacket;
        if (packetLength > cbPacket)
            packetLength = cbPacket;

        if (multiplePayloads)
        {
            if (off >= cbPacket)
                return -1;
            payloadCount      = pPacket[off] & 0x3F;
            payloadLengthType = (pPacket[off] >> 6) & 0x03;
            off += 1;
        }
        else
        {
            payloadCount = 1;
        }
    }

    // Padding sits at the end of the packet and is not payload.
    dataEnd = (paddingLength < packetLength) ? packetLength - paddingLength : packetLength;

    for (i = 0; i < payloadCount; i++)
    {
        unsigned int streamNumberType = (propertyFlags >> 6) & 0x03;
        unsigned int mediaObjectType  = (propertyFlags >> 4) & 0x03;
        unsigned int offsetType       = (propertyFlags >> 2) & 0x03;
        unsigned int replicatedType   = propertyFlags & 0x03;
        unsigned int replicatedLength;
        unsigned int payloadLength;
        unsigned int isAudio;

        if (off + varlen_size(streamNumberType) + varlen_size(mediaObjectType) +
                   varlen_size(offsetType) + varlen_size(replicatedType) > dataEnd)
            break;

        // Stream number: low 7 bits, top bit is the key-frame marker. A .wma has one stream, so
        // rather than matching a number we simply take every payload -- the file's only stream is
        // the audio one, and a mismatch would mean the header lied.
        isAudio = 1;
        (void)read_varlen(pPacket, streamNumberType, &off);
        (void)read_varlen(pPacket, mediaObjectType, &off);
        (void)read_varlen(pPacket, offsetType, &off);
        replicatedLength = read_varlen(pPacket, replicatedType, &off);

        if (replicatedLength == 1)
        {
            // Compressed payload: the "replicated data" byte is a presentation-time delta and the
            // payload is a run of sub-payloads, each preceded by its length. WMA encoders do not
            // emit these for audio, but handle them rather than mis-parse if one shows up.
            unsigned int subEnd;

            if (off + 1 > dataEnd)
                break;
            off += 1;

            if (payloadLengthType)
            {
                if (off + varlen_size(payloadLengthType) > dataEnd)
                    break;
                payloadLength = read_varlen(pPacket, payloadLengthType, &off);
            }
            else
            {
                payloadLength = dataEnd - off;
            }

            subEnd = off + payloadLength;
            if (subEnd > dataEnd)
                subEnd = dataEnd;

            while (off < subEnd)
            {
                unsigned int subLen = pPacket[off++];

                if (off + subLen > subEnd)
                    break;
                if (isAudio && written + subLen <= cbOut)
                {
                    memcpy(pOut + written, pPacket + off, subLen);
                    written += subLen;
                }
                off += subLen;
            }

            off = subEnd;
            continue;
        }

        if (off + replicatedLength > dataEnd)
            break;
        off += replicatedLength;

        if (payloadLengthType)
        {
            if (off + varlen_size(payloadLengthType) > dataEnd)
                break;
            payloadLength = read_varlen(pPacket, payloadLengthType, &off);
        }
        else
        {
            // Single payload: it runs to the end of the packet data.
            payloadLength = (dataEnd > off) ? dataEnd - off : 0;
        }

        if (off + payloadLength > dataEnd)
            payloadLength = (dataEnd > off) ? dataEnd - off : 0;

        if (isAudio && payloadLength && written + payloadLength <= cbOut)
        {
            memcpy(pOut + written, pPacket + off, payloadLength);
            written += payloadLength;
        }

        off += payloadLength;
    }

    *pcbOut = written;

    return 0;
}
