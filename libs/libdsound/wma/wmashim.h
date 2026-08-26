/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * wmashim.h - minimal replacement for ffmpeg's avcodec.h / common.h /
 *             dsputil.h / bswap.h, providing ONLY what wmadec.c, fft.c and
 *             mdct.c actually reference.
 *
 * Self-contained: depends only on the C standard library.  It is the
 * dependency-free C port of ffmpeg's WMA v1/v2 decoder used by the Xbox
 * audio library.
 */
#ifndef WMASHIM_H
#define WMASHIM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---- math constants ------------------------------------------------------- */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- memory (ffmpeg av_* -> libc) ----------------------------------------- */
static inline void *av_malloc(unsigned int size)   { return malloc(size ? size : 1); }
static inline void *av_mallocz(unsigned int size)   { void *p = calloc(1, size ? size : 1); return p; }
static inline void *av_realloc(void *ptr, unsigned int size) { return realloc(ptr, size ? size : 1); }
static inline void  av_free(void *ptr)              { free(ptr); }
static inline void  av_freep(void *arg)             { void **p = (void **)arg; free(*p); *p = NULL; }

/* ---- logging / trace (no-ops) --------------------------------------------- */
#define AV_LOG_ERROR 0
#define AV_LOG_INFO  0
#define AV_LOG_DEBUG 0
#define av_log(...)  do {} while (0)
#define dprintf(...) do {} while (0)
#define tprintf(...) do {} while (0)

/* ---- alignment (perf detail only; correctness-neutral) -------------------- */
#define DECLARE_ALIGNED_16(t, v) t v

/* ---- integer log2 --------------------------------------------------------- */
static inline int av_log2(unsigned int v)
{
    int n = 0;
    while (v >>= 1) n++;
    return n;
}

/* ---- codec enums / structs (subset) --------------------------------------- */
enum CodecType { CODEC_TYPE_AUDIO = 1 };
enum CodecID   { CODEC_ID_WMAV1 = 1, CODEC_ID_WMAV2 = 2 };

struct AVCodecContext;

typedef struct AVCodec {
    const char *name;
    int type;
    int id;
    int priv_data_size;
    int (*init)(struct AVCodecContext *);
    int (*encode)(struct AVCodecContext *, uint8_t *, int, void *);
    int (*close)(struct AVCodecContext *);
    int (*decode)(struct AVCodecContext *, void *, int *, uint8_t *, int);
} AVCodec;

typedef struct AVCodecContext {
    void          *priv_data;
    int            sample_rate;
    int            channels;
    int            bit_rate;
    int            block_align;
    uint8_t       *extradata;
    int            extradata_size;
    const AVCodec *codec;
} AVCodecContext;

/* ---- FFT / MDCT (structs owned by fft.c / mdct.c) -------------------------- */
typedef float FFTSample;

struct MDCTContext;

typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;

typedef struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    FFTComplex *exptab;
    FFTComplex *exptab1; /* unused (SSE path removed) */
    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
    void (*imdct_calc)(struct MDCTContext *s, FFTSample *output,
                       const FFTSample *input, FFTSample *tmp);
} FFTContext;

int  ff_fft_init(FFTContext *s, int nbits, int inverse);
void ff_fft_permute(FFTContext *s, FFTComplex *z);
void ff_fft_calc_c(FFTContext *s, FFTComplex *z);
void ff_fft_end(FFTContext *s);

static inline void ff_fft_calc(FFTContext *s, FFTComplex *z) { s->fft_calc(s, z); }

typedef struct MDCTContext {
    int n;      /* size of MDCT (number of input data * 2) */
    int nbits;  /* n = 2^nbits */
    FFTSample *tcos;
    FFTSample *tsin;
    FFTContext fft;
} MDCTContext;

int  ff_mdct_init(MDCTContext *s, int nbits, int inverse);
void ff_imdct_calc(MDCTContext *s, FFTSample *output,
                   const FFTSample *input, FFTSample *tmp);
void ff_mdct_calc(MDCTContext *s, FFTSample *out,
                  const FFTSample *input, FFTSample *tmp);
void ff_mdct_end(MDCTContext *s);

/* ---- DSPContext: only vector_fmul_add_add is used by wmadec --------------- */
static inline void ff_vector_fmul_add_add_c(float *dst, const float *src0,
                                            const float *src1, const float *src2,
                                            int src3, int len, int step)
{
    int i;
    for (i = 0; i < len; i++)
        dst[i * step] = src0[i] * src1[i] + src2[i] + src3;
}

typedef struct DSPContext {
    void (*vector_fmul_add_add)(float *dst, const float *src0, const float *src1,
                                const float *src2, int src3, int len, int step);
} DSPContext;

static inline void dsputil_init(DSPContext *c, AVCodecContext *avctx)
{
    (void)avctx;
    c->vector_fmul_add_add = ff_vector_fmul_add_add_c;
}

/* ---- bitstream / VLC subset ----------------------------------------------- */
#include "get_bits.h"

#endif /* WMASHIM_H */
