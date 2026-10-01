/* t_memcpy.c -- correctness sweep for the SSE1 memcpy (libs/libc/xbox/
 * sse1_memcpy.c). That implementation branches heavily by size (<=16, <=32,
 * <=64, >64 head-align + 64B blocks + tail) and by store path (movaps loop,
 * rep movsd from REP_MIN=512, non-temporal block-prefetch from NT_THRESHOLD=
 * 128K), so a regression can hide in one class/alignment while the trivial
 * "memcpy 6 bytes" check in t_cstring still passes. This sweeps every size
 * class across many src/dst alignments and checks four things per case:
 *   - the return value is dst
 *   - dst[0..n) equals the source pattern exactly (no under/over copy)
 *   - the GUARD bytes on either side of dst are untouched (no overrun)
 *   - the source is unmodified
 * It also exercises the streaming entry point sse1_memcpy_nt and the n==0 and
 * dst==src corner cases. */
#include "rxdk_test.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* libc entry points under test. memcpy is the auto (cached/NT-by-size) variant;
 * sse1_memcpy_nt forces the non-temporal streaming path for every n > 64. */
extern void *sse1_memcpy_nt(void *, const void *, size_t);

#define GUARD 32u
#define MAXN  (256u * 1024u)          /* > NT_THRESHOLD (128K) so the NT path runs */
#define POISON 0x5A

static unsigned char *sbase, *dbase; /* 64-byte-aligned working bases */

static unsigned char patt(size_t i) { return (unsigned char)(0xA5u ^ (i * 31u) ^ (i >> 3)); }

/* First-failure diagnostics (printed once in the CHECK description). */
static size_t f_n; static unsigned f_so, f_do; static int f_why; /* 1=ret 2=data 3=guard 4=src */

/* Run one case; nt selects the streaming entry point. Returns 1 on success. */
static int one(size_t n, unsigned so, unsigned dof, int nt)
{
    unsigned char *s = sbase + 64 + so;
    unsigned char *d = dbase + 64 + dof;
    unsigned i;

    memset(d - GUARD, POISON, n + 2 * GUARD);       /* poison dst + both guards */
    for (size_t k = 0; k < n; k++) s[k] = patt(k);  /* source pattern           */

    void *r = nt ? sse1_memcpy_nt(d, s, n) : memcpy(d, s, n);

    if (r != d)                                    { f_why = 1; goto fail; }
    for (size_t k = 0; k < n; k++) if (d[k] != patt(k)) { f_why = 2; goto fail; }
    for (i = 0; i < GUARD; i++) if (d[-1 - (int)i] != POISON) { f_why = 3; goto fail; }
    for (i = 0; i < GUARD; i++) if (d[n + i]       != POISON) { f_why = 3; goto fail; }
    for (size_t k = 0; k < n; k++) if (s[k] != patt(k)) { f_why = 4; goto fail; }
    return 1;
fail:
    if (!f_n && !f_so && !f_do) { f_n = n; f_so = so; f_do = dof; }  /* keep first */
    return 0;
}

/* Alignment offsets that exercise 4/8/16/32/64-byte boundaries and odd cases. */
static const unsigned kAlign[] = { 0, 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 48, 63 };
#define NAL ((unsigned)(sizeof(kAlign) / sizeof(kAlign[0])))

/* Sizes hitting every class + boundary of the implementation. */
static const size_t kSmall[] = {
    1,2,3,4,5,6,7,8,9,15,16,17,24,31,32,33,48,63,64,65,
    96,127,128,129,200,255,256,257,384,511,512,513,600,1000,
    2000,4095,4096,8191,8192,16384
};
#define NSMALL ((unsigned)(sizeof(kSmall) / sizeof(kSmall[0])))

/* Large sizes crossing REP_MIN and the 128K NT_THRESHOLD. */
static const size_t kLarge[] = {
    32768, 65536, 131071, 131072, 131072 + 64, 200000, 262144
};
#define NLARGE ((unsigned)(sizeof(kLarge) / sizeof(kLarge[0])))
static const unsigned kLargeAlign[] = { 0, 1, 8, 15, 16, 31 };
#define NLA ((unsigned)(sizeof(kLargeAlign) / sizeof(kLargeAlign[0])))

int main(void)
{
    unsigned char *sbuf = (unsigned char *)malloc(MAXN + 256);
    unsigned char *dbuf = (unsigned char *)malloc(MAXN + 256);
    CHECK(sbuf != NULL && dbuf != NULL, "allocate two 256K+ working buffers");
    if (!sbuf || !dbuf) return 0;
    sbase = (unsigned char *)(((uintptr_t)sbuf + 63) & ~(uintptr_t)63);
    dbase = (unsigned char *)(((uintptr_t)dbuf + 63) & ~(uintptr_t)63);

    long cases = 0, fails = 0;

    /* memcpy (auto): every small/mid size x every alignment pair. */
    for (unsigned si = 0; si < NSMALL; si++)
        for (unsigned a = 0; a < NAL; a++)
            for (unsigned b = 0; b < NAL; b++) {
                cases++; if (!one(kSmall[si], kAlign[a], kAlign[b], 0)) fails++;
            }
    if (fails)
        DbgPrint("[T] INFO first memcpy fail: n=%lu so=%u do=%u why=%d\n",
                 (unsigned long)f_n, f_so, f_do, f_why);
    CHECK(fails == 0, "memcpy: all small/mid size x alignment cases byte-exact");

    /* memcpy (auto): large sizes crossing REP_MIN + the 128K NT path. */
    long lc = 0, lf = 0;
    for (unsigned si = 0; si < NLARGE; si++)
        for (unsigned a = 0; a < NLA; a++)
            for (unsigned b = 0; b < NLA; b++) {
                lc++; if (!one(kLarge[si], kLargeAlign[a], kLargeAlign[b], 0)) lf++;
            }
    if (lf)
        DbgPrint("[T] INFO first large fail: n=%lu so=%u do=%u why=%d\n",
                 (unsigned long)f_n, f_so, f_do, f_why);
    CHECK(lf == 0, "memcpy: large sizes incl >128K non-temporal path byte-exact");

    /* sse1_memcpy_nt: forced streaming path, sizes > 64 across alignments. */
    long nc = 0, nf = 0;
    f_n = 0; f_so = 0; f_do = 0;
    for (unsigned si = 0; si < NSMALL; si++) {
        if (kSmall[si] <= 64) continue;
        for (unsigned a = 0; a < NLA; a++)
            for (unsigned b = 0; b < NLA; b++) {
                nc++; if (!one(kSmall[si], kLargeAlign[a], kLargeAlign[b], 1)) nf++;
            }
    }
    for (unsigned si = 0; si < NLARGE; si++)
        for (unsigned a = 0; a < NLA; a++) {
            nc++; if (!one(kLarge[si], kLargeAlign[a], kLargeAlign[0], 1)) nf++;
        }
    if (nf)
        DbgPrint("[T] INFO first nt fail: n=%lu so=%u do=%u why=%d\n",
                 (unsigned long)f_n, f_so, f_do, f_why);
    CHECK(nf == 0, "sse1_memcpy_nt: forced streaming path byte-exact");

    /* n == 0 must be a no-op that returns dst and touches nothing. */
    {
        unsigned char *d = dbase + 64;
        memset(d - GUARD, POISON, GUARD * 2);
        void *r = memcpy(d, sbase + 64, 0);
        int ok = (r == d);
        for (unsigned i = 0; i < GUARD; i++) if (d[-1 - (int)i] != POISON || d[i] != POISON) ok = 0;
        CHECK(ok, "memcpy: n=0 returns dst and writes nothing");
    }

    /* dst == src is defined to be harmless (returns dst, data unchanged). */
    {
        unsigned char *p = dbase + 64;
        for (unsigned i = 0; i < 200; i++) p[i] = patt(i);
        void *r = memcpy(p, p, 200);
        int ok = (r == p);
        for (unsigned i = 0; i < 200; i++) if (p[i] != patt(i)) ok = 0;
        CHECK(ok, "memcpy: dst==src is harmless");
    }

    DbgPrint("[T] INFO memcpy cases: auto=%ld large=%ld nt=%ld (fails %ld/%ld/%ld)\n",
             cases, lc, nc, fails, lf, nf);

    free(sbuf);
    free(dbuf);
    CHECK_DONE("memcpy");
    return 0;
}
