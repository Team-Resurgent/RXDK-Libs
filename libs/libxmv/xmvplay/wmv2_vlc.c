//------------------------------------------------------------------------------
// wmv2_vlc.c -- see wmv2_vlc.h.
//------------------------------------------------------------------------------

#include <xtl.h>
#include "decoder.h"        // XmvVideoCore + PeekBits/SkipBits
#include "wmv2_vlc.h"

// Insertion sort of the entry list ascending by code (n <= ~1100, once at init).
static void sort_entries(Wmv2VlcEntry *e, int n)
{
    int i, j;
    for (i = 1; i < n; i++) {
        Wmv2VlcEntry key = e[i];
        j = i - 1;
        while (j >= 0 && e[j].code > key.code) {
            e[j + 1] = e[j];
            j--;
        }
        e[j + 1] = key;
    }
}

int Wmv2VlcBuildFromLengths(Wmv2Vlc *v, const uint8_t *lens, const uint16_t *syms, int n)
{
    uint32_t code = 0;
    int i, k = 0, maxlen = 0;

    v->entries = (Wmv2VlcEntry *)malloc(sizeof(Wmv2VlcEntry) * n);
    if (!v->entries)
        return -1;

    // Mirror ff_vlc_init_from_lengths: assign `code` (already left-justified)
    // to each nonzero-length symbol in order, then advance by 1<<(32-len).
    for (i = 0; i < n; i++) {
        int len = lens[i];
        if (len > 0) {
            v->entries[k].code = code;
            v->entries[k].sym  = syms ? syms[i] : (uint16_t)i;
            v->entries[k].len  = (uint8_t)len;
            k++;
            if (len > maxlen)
                maxlen = len;
            code += 1u << (32 - len);
        }
        // len == 0: skip, no code advance (matches FFmpeg's `continue`).
    }

    v->n = k;
    v->maxlen = maxlen ? maxlen : 1;
    sort_entries(v->entries, k);
    return 0;
}

int Wmv2VlcBuildExplicit(Wmv2Vlc *v, const uint32_t table[][2], int n)
{
    int i, k = 0, maxlen = 0;

    v->entries = (Wmv2VlcEntry *)malloc(sizeof(Wmv2VlcEntry) * n);
    if (!v->entries)
        return -1;

    for (i = 0; i < n; i++) {
        int len = (int)table[i][1];
        if (len > 0) {
            v->entries[k].code = table[i][0] << (32 - len);  // left-justify
            v->entries[k].sym  = (uint16_t)i;
            v->entries[k].len  = (uint8_t)len;
            k++;
            if (len > maxlen)
                maxlen = len;
        }
    }

    v->n = k;
    v->maxlen = maxlen ? maxlen : 1;
    sort_entries(v->entries, k);
    return 0;
}

void Wmv2VlcFree(Wmv2Vlc *v)
{
    if (v->entries)
        free(v->entries);
    v->entries = NULL;
    v->n = 0;
}

int Wmv2VlcDecode(XmvVideoCore *core, const Wmv2Vlc *v)
{
    // Peek maxlen bits, left-justify to 32 bits, binary-search the largest code
    // <= the peeked value (the matching prefix-code entry), then consume its len.
    uint32_t acc = PeekBits(core, v->maxlen) << (32 - v->maxlen);
    int lo = 0, hi = v->n - 1, best = 0;

    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        if (v->entries[mid].code <= acc) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    SkipBits(core, v->entries[best].len);
    return v->entries[best].sym;
}
