//------------------------------------------------------------------------------
// wmv2_tables.h -- decls for the WMV2 P-frame VLC tables (wmv2_tables.c, copied
// verbatim from FFmpeg libavcodec/msmpeg4data.c).
//------------------------------------------------------------------------------
#ifndef RXDK_WMV2_TABLES_H
#define RXDK_WMV2_TABLES_H

#include <stdint.h>

#define MSMPEG4_MV_TABLES_NB_ELEMS 1100

// Motion-vector VLC: symbol = (mx<<8)|my, length in the matching _lens array.
// Symbol 0 == escape (raw 6+6 bit mv). Codes are canonical from the lengths.
extern const uint16_t ff_msmp4_mv_table0[MSMPEG4_MV_TABLES_NB_ELEMS];
extern const uint8_t  ff_msmp4_mv_table0_lens[MSMPEG4_MV_TABLES_NB_ELEMS];
extern const uint16_t ff_msmp4_mv_table1[MSMPEG4_MV_TABLES_NB_ELEMS];
extern const uint8_t  ff_msmp4_mv_table1_lens[MSMPEG4_MV_TABLES_NB_ELEMS];

// P macroblock-type + CBP VLC: [symbol(=array index 0..127)][{code, len}].
// Explicit codes (right-justified). Selected by cbp_table_index 0..3.
extern const uint32_t ff_table_mb_non_intra[128][2];
extern const uint32_t table_mb_non_intra2[128][2];
extern const uint32_t table_mb_non_intra3[128][2];
extern const uint32_t table_mb_non_intra4[128][2];
extern const uint32_t (*const ff_wmv2_inter_table[4])[2];

// cbp_table_index from qscale + the 2-bit cbp_index (wmv2.h).
static inline int wmv2_get_cbp_table_index(int qscale, int cbp_index)
{
    static const uint8_t map[3][3] = {
        { 0, 2, 1 },
        { 1, 0, 2 },
        { 2, 1, 0 },
    };
    return map[(qscale > 10) + (qscale > 20)][cbp_index];
}

#endif // RXDK_WMV2_TABLES_H
