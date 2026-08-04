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

void XactWmaFreeBank(void *pBank)
{
    free(pBank);
}

// ---- RXWM bank transcode-on-load --------------------------------------------------------------
//
// xactbld emits a wave bank that contains WMA as a private "RXWM" container (magic 'RXWM') instead
// of the normal .xwb, because the leak's wave-bank mini-format has only a 1-bit PCM/ADPCM tag with
// no room for WMA + its extradata. On load, the engine calls XactMaybeTranscodeWmaBank, which
// decodes every WMA entry to PCM and rebuilds a standard .xwb (WBND) in memory that the normal
// CWaveBank parser then consumes. A plain (non-RXWM) buffer is passed through untouched.
//
// RXWM layout (all little-endian):
//   char  magic[4]      = "RXWM"
//   u32   version       = 1
//   u32   entryCount
//   char  bankName[16]
//   per entry:
//     u16 formatTag  (1 = PCM, 0x0160 = WMAv1, 0x0161 = WMAv2)
//     u16 channels
//     u32 samplesPerSec
//     u32 avgBytesPerSec
//     u16 blockAlign
//     u16 bitsPerSample
//     u16 extraDataSize
//     u16 reserved
//     u32 dataSize
//     u8  extraData[extraDataSize]
//     u8  data[dataSize]        (PCM if formatTag==1, else WMA packets)

#define XWB_SIGNATURE 0x444E4257u  /* 'WBND' on disk */
#define XWB_VERSION   2u
#define XWB_ALIGN     2048u
#define XWB_NAMELEN   16

static unsigned int rd_u32(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }
static unsigned int rd_u16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static void wr_u32(unsigned char *p, unsigned int v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF; }
static unsigned int align_up(unsigned int v, unsigned int a) { return (v + a - 1) & ~(a - 1); }

int XactMaybeTranscodeWmaBank(const unsigned char *pvData, unsigned int dwSize, void **ppOut, unsigned int *pcbOut)
{
    unsigned int version, entryCount, i, cursor;
    unsigned char **pcm = NULL;      // per-entry PCM buffer (owned here)
    unsigned int *pcmLen = NULL;     // per-entry PCM byte length
    unsigned int *chans = NULL, *rates = NULL;
    unsigned int dataSeg, hdrSize, entTblSize, total, dpos, epos;
    unsigned char *out = NULL;
    char bankName[XWB_NAMELEN];

    if (ppOut) *ppOut = NULL;
    if (pcbOut) *pcbOut = 0;
    if (!ppOut || !pcbOut || dwSize < 28 ||
        pvData[0] != 'R' || pvData[1] != 'X' || pvData[2] != 'W' || pvData[3] != 'M')
        return 0;   // not a WMA bank -> caller uses the buffer as-is

    version = rd_u32(pvData + 4);
    entryCount = rd_u32(pvData + 8);
    (void)version;
    memcpy(bankName, pvData + 12, XWB_NAMELEN);

    if (entryCount == 0 || entryCount > 4096)
        return 0;

    pcm = (unsigned char **)calloc(entryCount, sizeof(*pcm));
    pcmLen = (unsigned int *)calloc(entryCount, sizeof(*pcmLen));
    chans = (unsigned int *)calloc(entryCount, sizeof(*chans));
    rates = (unsigned int *)calloc(entryCount, sizeof(*rates));
    if (!pcm || !pcmLen || !chans || !rates)
        goto fail;

    // Pass 1: decode/copy every entry to PCM.
    cursor = 28;
    for (i = 0; i < entryCount; i++)
    {
        unsigned int fmtTag, ch, rate, abps, blockAlign, bits, extraSize, dataSize;
        const unsigned char *extra, *data;
        if (cursor + 24 > dwSize)
            goto fail;
        fmtTag     = rd_u16(pvData + cursor + 0);
        ch         = rd_u16(pvData + cursor + 2);
        rate       = rd_u32(pvData + cursor + 4);
        abps       = rd_u32(pvData + cursor + 8);
        blockAlign = rd_u16(pvData + cursor + 12);
        bits       = rd_u16(pvData + cursor + 14);
        extraSize  = rd_u16(pvData + cursor + 16);
        /* +18 reserved u16 */
        dataSize   = rd_u32(pvData + cursor + 20);
        cursor += 24;
        if (cursor + extraSize + dataSize > dwSize)
            goto fail;
        extra = pvData + cursor;
        data  = pvData + cursor + extraSize;
        cursor += extraSize + dataSize;

        chans[i] = ch;
        rates[i] = rate;
        (void)bits;

        if (fmtTag == 1)
        {
            // Already PCM -> copy through.
            pcm[i] = (unsigned char *)malloc(dataSize ? dataSize : 1);
            if (!pcm[i]) goto fail;
            memcpy(pcm[i], data, dataSize);
            pcmLen[i] = dataSize;
        }
        else
        {
            short *decoded = NULL;
            int decodedBytes = 0;
            if (XactWmaDecode((unsigned short)fmtTag, (int)ch, (int)rate, (int)(abps * 8),
                              (int)blockAlign, extra, (int)extraSize, data, (int)dataSize,
                              &decoded, &decodedBytes) != 0)
                goto fail;
            pcm[i] = (unsigned char *)decoded;   // XactWmaDecode malloc'd it
            pcmLen[i] = (unsigned int)decodedBytes;
        }
    }

    // Pass 2: lay out a standard .xwb (WBND). Header 36 + entryCount*20 + aligned data segment.
    hdrSize = 20 + XWB_NAMELEN;         // 5*u32 + name[16] = 36
    entTblSize = entryCount * 20;
    dataSeg = 0;
    for (i = 0; i < entryCount; i++)
        dataSeg = align_up(dataSeg, XWB_ALIGN) + pcmLen[i];
    total = hdrSize + entTblSize + dataSeg;

    out = (unsigned char *)malloc(total);
    if (!out)
        goto fail;
    memset(out, 0, total);

    wr_u32(out + 0, XWB_SIGNATURE);
    wr_u32(out + 4, XWB_VERSION);
    wr_u32(out + 8, 0);                 // dwFlags
    wr_u32(out + 12, entryCount);
    wr_u32(out + 16, XWB_ALIGN);
    memcpy(out + 20, bankName, XWB_NAMELEN);

    epos = hdrSize;                     // entry table cursor
    dpos = 0;                           // offset within the data segment
    for (i = 0; i < entryCount; i++)
    {
        unsigned int fmt, playStart;
        dpos = align_up(dpos, XWB_ALIGN);
        playStart = dpos;
        // mini-format: tag(1b)=PCM(0) | channels<<1 | rate<<4 | bits(16-bit)=1<<31
        fmt = ((chans[i] & 0x7u) << 1) | ((rates[i] & 0x7FFFFFFu) << 4) | (1u << 31);
        wr_u32(out + epos + 0, fmt);
        wr_u32(out + epos + 4, playStart);
        wr_u32(out + epos + 8, pcmLen[i]);
        wr_u32(out + epos + 12, 0);     // loopStart
        wr_u32(out + epos + 16, 0);     // loopLen
        epos += 20;

        if (pcmLen[i])
            memcpy(out + hdrSize + entTblSize + dpos, pcm[i], pcmLen[i]);
        dpos += pcmLen[i];
    }

    for (i = 0; i < entryCount; i++)
        free(pcm[i]);
    free(pcm); free(pcmLen); free(chans); free(rates);

    *ppOut = out;
    *pcbOut = total;
    return 1;

fail:
    if (pcm)
        for (i = 0; i < entryCount; i++)
            free(pcm[i]);
    free(pcm); free(pcmLen); free(chans); free(rates); free(out);
    return 0;
}
