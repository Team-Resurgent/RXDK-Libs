/*
 * get_bits.c - init_vlc / free_vlc, the Huffman-table builder used by get_vlc2.
 *
 * This is the canonical ffmpeg VLC builder (libavcodec/bitstream.c), reduced to
 * the big-endian (non-LE) path that the WMA decoder needs.  The table layout it
 * produces matches the GET_VLC reader macro in get_bits.h:
 *   table[i][0] = symbol   (or base index of a sub-table when table[i][1] < 0)
 *   table[i][1] = code length in bits (>0 = leaf; <0 = sub-table depth)
 */
#include <stdint.h>
#include <stdlib.h>
#include "wmashim.h"   /* for av_malloc/av_free/av_realloc + VLC (via get_bits.h) */

#define GET_DATA(v, table, i, wrap, size) \
{ \
    const uint8_t *ptr = (const uint8_t *)(table) + (i) * (wrap); \
    switch (size) { \
    case 1:  v = *(const uint8_t  *)ptr; break; \
    case 2:  v = *(const uint16_t *)ptr; break; \
    default: v = *(const uint32_t *)ptr; break; \
    } \
}

static int alloc_table(VLC *vlc, int size)
{
    int index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) {
        vlc->table_allocated += (1 << vlc->bits);
        vlc->table = av_realloc(vlc->table,
                                sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

static int build_table(VLC *vlc, int table_nb_bits,
                       int nb_codes,
                       const void *bits, int bits_wrap, int bits_size,
                       const void *codes, int codes_wrap, int codes_size,
                       uint32_t code_prefix, int n_prefix)
{
    int i, j, k, n, table_size, table_index, nb, n1, index;
    uint32_t code;
    VLC_TYPE (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size);
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for (i = 0; i < table_size; i++) {
        table[i][1] = 0;   /* bits  */
        table[i][0] = -1;  /* codes */
    }

    /* first pass: map codes and compute auxiliary table sizes */
    for (i = 0; i < nb_codes; i++) {
        GET_DATA(n, bits, i, bits_wrap, bits_size);
        GET_DATA(code, codes, i, codes_wrap, codes_size);
        if (n <= 0)
            continue;                 /* tables may have holes */
        n -= n_prefix;
        if (n > 0 && code_prefix == (code >> n)) {
            if (n <= table_nb_bits) {
                /* fits directly in this table */
                j  = (code << (table_nb_bits - n)) & (table_size - 1);
                nb = 1 << (table_nb_bits - n);
                for (k = 0; k < nb; k++) {
                    if (table[j][1] /* bits */ != 0) {
                        /* incorrect codes */
                        return -1;
                    }
                    table[j][1] = n;   /* bits  */
                    table[j][0] = i;   /* symbol */
                    j++;
                }
            } else {
                /* mark that a sub-table of depth (n - table_nb_bits) is needed */
                n -= table_nb_bits;
                j  = (code >> n) & ((1 << table_nb_bits) - 1);
                n1 = -table[j][1]; /* bits */
                if (n > n1)
                    n1 = n;
                table[j][1] = -n1; /* bits */
            }
        }
    }

    /* second pass: build the sub-tables recursively */
    for (i = 0; i < table_size; i++) {
        n = table[i][1]; /* bits */
        if (n < 0) {
            n = -n;
            if (n > table_nb_bits) {
                n = table_nb_bits;
                table[i][1] = -n; /* bits */
            }
            index = build_table(vlc, n, nb_codes,
                                 bits, bits_wrap, bits_size,
                                 codes, codes_wrap, codes_size,
                                 (code_prefix << table_nb_bits) | i,
                                 n_prefix + table_nb_bits);
            if (index < 0)
                return -1;
            table = &vlc->table[table_index]; /* realloc may have moved it */
            table[i][0] = index;              /* code = sub-table base */
        }
    }
    return table_index;
}

int init_vlc(VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size,
             int flags)
{
    (void)flags;                 /* only the big-endian path is supported */
    vlc->bits = nb_bits;
    vlc->table = NULL;
    vlc->table_allocated = 0;
    vlc->table_size = 0;

    if (build_table(vlc, nb_bits, nb_codes,
                    bits, bits_wrap, bits_size,
                    codes, codes_wrap, codes_size,
                    0, 0) < 0) {
        av_free(vlc->table);
        vlc->table = NULL;
        return -1;
    }
    return 0;
}

void free_vlc(VLC *vlc)
{
    av_free(vlc->table);
    vlc->table = NULL;
}
