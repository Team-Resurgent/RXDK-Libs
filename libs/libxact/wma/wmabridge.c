// RXDK 5849 uplift: C bridge over the ported ffmpeg WMA v1/v2 decoder. Exposes a single
// decode-whole-entry entry point the (C++) XACT engine calls to turn a WMA wave-bank entry into
// interleaved signed-16-bit PCM at load time ("decode on load"), so the rest of the engine only
// ever sees PCM. Pure C, built in the minimal (picolibc) WMA slice.

#include "wmashim.h"
#include "wmabridge.h"
#include <stdlib.h>
#include <string.h>

// Defined in wmadec.c.
extern AVCodec wmav1_decoder;
extern AVCodec wmav2_decoder;

int XactWmaDecode(unsigned short formatTag,
                  int channels, int sampleRate, int bitRate, int blockAlign,
                  const unsigned char *extradata, int extradataSize,
                  const unsigned char *wmaData, int wmaDataSize,
                  short **ppPcm, int *pcbPcm)
{
    AVCodecContext avctx;
    const AVCodec *codec;
    void *priv = NULL;
    unsigned char *inbuf = NULL;   // one packet + trailing pad (the bit reader over-reads a few bytes)
    short *scratch = NULL;         // per-superframe decode output
    short *out = NULL;             // growable accumulated PCM
    int outCap = 0, outLen = 0;    // bytes
    int pos;

    if (!ppPcm || !pcbPcm)
        return -1;
    *ppPcm = NULL;
    *pcbPcm = 0;
    if (blockAlign <= 0 || channels <= 0 || !wmaData || wmaDataSize < blockAlign)
        return -1;

    codec = (formatTag == 0x0160) ? &wmav1_decoder : &wmav2_decoder;

    memset(&avctx, 0, sizeof(avctx));
    avctx.sample_rate    = sampleRate;
    avctx.channels       = channels;
    avctx.bit_rate       = bitRate;
    avctx.block_align    = blockAlign;
    avctx.extradata      = (uint8_t *)extradata;
    avctx.extradata_size = extradataSize;
    avctx.codec          = codec;

    priv = calloc(1, codec->priv_data_size);
    inbuf = (unsigned char *)malloc(blockAlign + 16);
    scratch = (short *)malloc(1 << 18);   // 256 KB: comfortably one superframe of s16 output
    if (!priv || !inbuf || !scratch)
        goto fail;
    avctx.priv_data = priv;

    if (codec->init(&avctx) < 0)
        goto fail;

    for (pos = 0; pos + blockAlign <= wmaDataSize; pos += blockAlign)
    {
        int outSize = 0;
        memcpy(inbuf, wmaData + pos, blockAlign);
        memset(inbuf + blockAlign, 0, 16);
        if (codec->decode(&avctx, scratch, &outSize, inbuf, blockAlign) < 0 || outSize <= 0)
            continue;

        if (outLen + outSize > outCap)
        {
            int newCap = outCap ? outCap : (1 << 20);
            short *grown;
            while (newCap < outLen + outSize)
                newCap *= 2;
            grown = (short *)realloc(out, newCap);
            if (!grown)
                goto fail;
            out = grown;
            outCap = newCap;
        }
        memcpy((unsigned char *)out + outLen, scratch, outSize);
        outLen += outSize;
    }

    if (codec->close)
        codec->close(&avctx);
    free(priv);
    free(inbuf);
    free(scratch);

    *ppPcm = out;
    *pcbPcm = outLen;
    return 0;

fail:
    if (avctx.priv_data && codec->close)
        codec->close(&avctx);
    free(priv);
    free(inbuf);
    free(scratch);
    free(out);
    return -1;
}

void XactWmaFree(short *pPcm)
{
    free(pPcm);
}
