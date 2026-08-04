/*
 * get_bits.h - minimal, self-contained subset of ffmpeg's bitstream reader.
 *
 * Extracted / distilled from ffmpeg (libavcodec/bitstream.h). Only the pieces
 * the WMA v1/v2 decoder needs are kept:
 *   - GetBitContext + the big-endian ALT_BITSTREAM_READER cache macros
 *   - init_get_bits / get_bits / get_bits1 / show_bits / skip_bits
 *   - align_get_bits / get_bits_count / skip_bits_long
 *   - VLC struct + GET_VLC / get_vlc2 (Huffman reader)
 *   - init_vlc / free_vlc  (implemented in get_bits.c)
 *
 * This is the plain-C, no-inline-asm, little-endian-host variant (the host that
 * runs the decoder is x86-64; WMA itself uses the *big-endian* bit reader, so
 * the cache is filled via a byte-swap of a 32-bit load).
 */
#ifndef GET_BITS_H
#define GET_BITS_H

#include <stdint.h>
#include <string.h>

/* ---- endian helper: WMA uses the big-endian bit reader -------------------- */
static inline uint32_t wma_bswap32(uint32_t x)
{
    return (x >> 24) | ((x >> 8) & 0xff00u) |
           ((x << 8) & 0xff0000u) | (x << 24);
}
static inline uint32_t wma_unaligned32(const void *v)
{
    uint32_t x;
    memcpy(&x, v, 4);        /* portable unaligned load */
    return x;
}
static inline int unaligned32_be(const void *v)
{
    return (int)wma_bswap32(wma_unaligned32(v));   /* host is little-endian */
}

/* portable NEG_*SR32 (no inline asm) */
#define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))

/* ---- context -------------------------------------------------------------- */
typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
} GetBitContext;

#define VLC_TYPE int16_t

typedef struct VLC {
    int bits;
    VLC_TYPE (*table)[2]; /* code, bits */
    int table_size, table_allocated;
} VLC;

/* ---- ALT_BITSTREAM_READER (big-endian) cache macros ----------------------- */
#define MIN_CACHE_BITS 25

#define OPEN_READER(name, gb) \
        int name##_index = (gb)->index; \
        int name##_cache = 0;

#define CLOSE_READER(name, gb) \
        (gb)->index = name##_index;

#define UPDATE_CACHE(name, gb) \
        name##_cache = unaligned32_be(((const uint8_t *)(gb)->buffer) + (name##_index >> 3)) << (name##_index & 0x07);

#define SKIP_CACHE(name, gb, num)   name##_cache <<= (num);
#define SKIP_COUNTER(name, gb, num) name##_index += (num);
#define SKIP_BITS(name, gb, num)    { SKIP_CACHE(name, gb, num) SKIP_COUNTER(name, gb, num) }
#define LAST_SKIP_BITS(name, gb, num)  SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) ;

#define SHOW_UBITS(name, gb, num)   NEG_USR32(name##_cache, num)
#define SHOW_SBITS(name, gb, num)   NEG_SSR32(name##_cache, num)
#define GET_CACHE(name, gb)         ((uint32_t)name##_cache)

static inline int get_bits_count(GetBitContext *s) { return s->index; }
static inline void skip_bits_long(GetBitContext *s, int n) { s->index += n; }

/* ---- basic reads ---------------------------------------------------------- */
static inline unsigned int get_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp = SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
    return tmp;
}

static inline unsigned int show_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp = SHOW_UBITS(re, s, n);
    return tmp;
}

static inline void skip_bits(GetBitContext *s, int n)
{
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
}

static inline unsigned int get_bits1(GetBitContext *s)
{
    int index = s->index;
    uint8_t result = s->buffer[index >> 3];
    result <<= (index & 0x07);
    result >>= 8 - 1;
    index++;
    s->index = index;
    return result;
}

static inline void init_get_bits(GetBitContext *s, const uint8_t *buffer, int bit_size)
{
    int buffer_size = (bit_size + 7) >> 3;
    if (buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = NULL;
    }
    s->buffer = buffer;
    s->size_in_bits = bit_size;
    s->buffer_end = buffer + buffer_size;
    s->index = 0;
}

static inline void align_get_bits(GetBitContext *s)
{
    int n = (-get_bits_count(s)) & 7;
    if (n) skip_bits(s, n);
}

/* ---- VLC ------------------------------------------------------------------ */
int  init_vlc(VLC *vlc, int nb_bits, int nb_codes,
              const void *bits, int bits_wrap, int bits_size,
              const void *codes, int codes_wrap, int codes_size,
              int flags);
void free_vlc(VLC *vlc);

#define GET_VLC(code, name, gb, table, bits, max_depth) \
{ \
    int n, index, nb_bits; \
    index = SHOW_UBITS(name, gb, bits); \
    code = table[index][0]; \
    n    = table[index][1]; \
    if (max_depth > 1 && n < 0) { \
        LAST_SKIP_BITS(name, gb, bits) \
        UPDATE_CACHE(name, gb) \
        nb_bits = -n; \
        index = SHOW_UBITS(name, gb, nb_bits) + code; \
        code = table[index][0]; \
        n    = table[index][1]; \
        if (max_depth > 2 && n < 0) { \
            LAST_SKIP_BITS(name, gb, nb_bits) \
            UPDATE_CACHE(name, gb) \
            nb_bits = -n; \
            index = SHOW_UBITS(name, gb, nb_bits) + code; \
            code = table[index][0]; \
            n    = table[index][1]; \
        } \
    } \
    SKIP_BITS(name, gb, n) \
}

static inline int get_vlc2(GetBitContext *s, VLC_TYPE (*table)[2],
                           int bits, int max_depth)
{
    int code;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    GET_VLC(code, re, s, table, bits, max_depth)
    CLOSE_READER(re, s)
    return code;
}

#endif /* GET_BITS_H */
