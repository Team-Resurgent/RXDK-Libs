//------------------------------------------------------------------------------
// wmv2_vlc.h -- minimal prefix-code (VLC) reader over the leak bit walker.
//
// Replaces FFmpeg's get_vlc2 + ff_vlc_init* for the two table kinds the WMV2
// P-frame path needs: canonical-from-lengths (motion vectors) and explicit
// {code,len} (P macroblock-type/CBP). Each table is flattened to a list of
// {left-justified-code, len, symbol} sorted ascending by code; decode peeks
// maxlen bits and binary-searches the largest code <= the peeked value (valid
// for any prefix code). Operates on XmvVideoCore's PeekBits/SkipBits.
//------------------------------------------------------------------------------
#ifndef RXDK_WMV2_VLC_H
#define RXDK_WMV2_VLC_H

#include <stdint.h>

struct XmvVideoCore;

typedef struct Wmv2VlcEntry {
    uint32_t code;   // code bits left-justified into 32 bits
    uint16_t sym;
    uint8_t  len;
} Wmv2VlcEntry;

typedef struct Wmv2Vlc {
    Wmv2VlcEntry *entries;   // sorted ascending by code
    int           n;
    int           maxlen;
} Wmv2Vlc;

// Build from FFmpeg-style canonical lengths (code += 1<<(32-len) per nonzero
// length, in symbol order). `syms` may be NULL (symbol = index). Returns 0 ok.
int  Wmv2VlcBuildFromLengths(Wmv2Vlc *v, const uint8_t *lens, const uint16_t *syms, int n);

// Build from explicit {code,len} pairs (table[i][0]=code, table[i][1]=len);
// symbol = i. Entries with len 0 are skipped.
int  Wmv2VlcBuildExplicit(Wmv2Vlc *v, const uint32_t table[][2], int n);

void Wmv2VlcFree(Wmv2Vlc *v);

// Decode one symbol, consuming its bits from the core's bit walker.
int  Wmv2VlcDecode(struct XmvVideoCore *core, const Wmv2Vlc *v);

#endif // RXDK_WMV2_VLC_H
