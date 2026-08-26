/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Streaming front end over the ported ffmpeg WMA v1/v2 decoder. Pure C, built
 * in the minimal (picolibc) WMA slice.
 */

#include "wmashim.h"
#include "wmastream.h"
#include <stdlib.h>
#include <string.h>

// Defined in wmadec.c.
extern AVCodec wmav1_decoder;
extern AVCodec wmav2_decoder;

// A superframe carries a 4-bit frame count, so at most 15 frames, plus one more carried over from
// the previous superframe's bit reservoir. frame_len is bounded by the decoder's own tables.
#define WMASTREAM_MAX_FRAMES_PER_SUPERFRAME 16
#define WMASTREAM_MAX_FRAME_LEN             2048

struct WmaStreamDecoder
{
    AVCodecContext  avctx;
    unsigned char * packet;         // one packet + pad (the bit reader over-reads a few bytes)
    int             packetCapacity;
    int             maxOutputBytes;
    unsigned short  formatTag;
    int             channels;
    int             sampleRate;
    int             bitRate;
    int             blockAlign;
    unsigned char * extradata;      // owned copy: the caller's may not outlive us
    int             extradataSize;
};

static int WmaStreamInitCodec(WmaStreamDecoder *d)
{
    const AVCodec *codec = (d->formatTag == 0x0160) ? &wmav1_decoder : &wmav2_decoder;

    memset(&d->avctx, 0, sizeof(d->avctx));
    d->avctx.sample_rate    = d->sampleRate;
    d->avctx.channels       = d->channels;
    d->avctx.bit_rate       = d->bitRate;
    d->avctx.block_align    = d->blockAlign;
    d->avctx.extradata      = d->extradata;
    d->avctx.extradata_size = d->extradataSize;
    d->avctx.codec          = codec;

    d->avctx.priv_data = calloc(1, codec->priv_data_size);
    if (!d->avctx.priv_data)
        return -1;

    if (codec->init(&d->avctx) < 0)
    {
        free(d->avctx.priv_data);
        d->avctx.priv_data = NULL;
        return -1;
    }

    return 0;
}

static void WmaStreamEndCodec(WmaStreamDecoder *d)
{
    if (d->avctx.priv_data)
    {
        if (d->avctx.codec && d->avctx.codec->close)
            d->avctx.codec->close(&d->avctx);
        free(d->avctx.priv_data);
        d->avctx.priv_data = NULL;
    }
}

int WmaStreamOpen(unsigned short formatTag,
                  int channels, int sampleRate, int bitRate, int blockAlign,
                  const unsigned char *extradata, int extradataSize,
                  WmaStreamDecoder **ppDecoder)
{
    WmaStreamDecoder *d;

    if (!ppDecoder)
        return -1;
    *ppDecoder = NULL;

    if (channels <= 0 || channels > 2 || sampleRate <= 0 || blockAlign <= 0)
        return -1;

    d = (WmaStreamDecoder *)calloc(1, sizeof(*d));
    if (!d)
        return -1;

    d->formatTag  = formatTag;
    d->channels   = channels;
    d->sampleRate = sampleRate;
    d->bitRate    = bitRate;
    d->blockAlign = blockAlign;

    if (extradata && extradataSize > 0)
    {
        d->extradata = (unsigned char *)malloc((size_t)extradataSize);
        if (!d->extradata)
            goto fail;
        memcpy(d->extradata, extradata, (size_t)extradataSize);
        d->extradataSize = extradataSize;
    }

    d->packetCapacity = blockAlign + 16;
    d->packet = (unsigned char *)malloc((size_t)d->packetCapacity);
    if (!d->packet)
        goto fail;

    if (WmaStreamInitCodec(d) < 0)
        goto fail;

    d->maxOutputBytes = channels * WMASTREAM_MAX_FRAME_LEN * 2 *
                        WMASTREAM_MAX_FRAMES_PER_SUPERFRAME;

    *ppDecoder = d;
    return 0;

fail:
    WmaStreamClose(d);
    return -1;
}

void WmaStreamClose(WmaStreamDecoder *d)
{
    if (!d)
        return;
    WmaStreamEndCodec(d);
    free(d->packet);
    free(d->extradata);
    free(d);
}

int WmaStreamMaxOutputBytes(const WmaStreamDecoder *d)
{
    return d ? d->maxOutputBytes : 0;
}

int WmaStreamDecode(WmaStreamDecoder *d,
                    const unsigned char *packet, int packetSize,
                    short *pcmOut, int pcmOutBytes, int *pcbPcm)
{
    int outSize = 0;

    if (pcbPcm)
        *pcbPcm = 0;

    if (!d || !packet || !pcmOut || !pcbPcm)
        return -1;
    if (!d->avctx.priv_data)    // a failed WmaStreamReset leaves the codec torn down
        return -1;
    if (packetSize <= 0 || packetSize > d->blockAlign)
        return -1;
    if (pcmOutBytes < d->maxOutputBytes)
        return -1;

    // The bit reader reads whole words past the end of the packet; give it the pad it expects.
    memcpy(d->packet, packet, (size_t)packetSize);
    memset(d->packet + packetSize, 0, (size_t)(d->packetCapacity - packetSize));

    if (d->avctx.codec->decode(&d->avctx, pcmOut, &outSize, d->packet, packetSize) < 0)
        return -1;

    *pcbPcm = outSize;
    return 0;
}

void WmaStreamReset(WmaStreamDecoder *d)
{
    if (!d)
        return;

    // The decoder keeps no public reset entry point, and its bit reservoir plus overlap-add
    // history live in priv_data, so tear the codec down and stand it back up. Failure leaves
    // priv_data NULL, which WmaStreamDecode rejects rather than dereferences.
    WmaStreamEndCodec(d);
    (void)WmaStreamInitCodec(d);
}
