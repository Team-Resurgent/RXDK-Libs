/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* A DPC runs on top of whatever thread it interrupted, and the kernel does not
   save that thread's FPU/SSE state for it (the XDK rule: no floating point at
   DPC time). The SDK's own DPCs call memcpy -- the USB input completion copies
   every gamepad report -- so libc's memcpy must not touch xmm there. Before
   that was fixed, a memcpy in a DPC overwrote the interrupted thread's xmm
   registers, which corrupted titles doing their float math in SSE.

   The SDK also runs every DPC it registers through RxdkRunDpcFpuSafe
   (RXDK_FPU_SAFE_DPC), which saves the FPU state around the routine, so SSE
   inside one -- a compiler-chosen xmm copy, a title's vertical blank callback --
   is safe too.

   The main thread holds a pattern in xmm0-xmm7 while a 1 ms periodic timer
   runs a DPC, then checks the registers. Original Xbox only. */
#include "rxdk_test.h"
#include <stdio.h>
#include <string.h>
#include <xboxkrnl/xboxkrnl.h>

void *sse1_memcpy(void *dst, const void *src, size_t n);                /* libc, always xmm */
void __stdcall RxdkRunDpcFpuSafe(void *Routine, void *Dpc, void *DeferredContext,
                                 void *SystemArgument1, void *SystemArgument2);  /* libxapi */

static KDPC g_dpc, g_dpc_safe;
static KTIMER g_timer;
static volatile int g_use_sse;
static volatile LONG g_dpc_count;
static volatile size_t g_size;
static volatile int g_irql_ok = 1;
static unsigned char g_src[8192], g_dst[8192];

/* The byte libc's memcpy reads to decide whether it may use SSE. */
static unsigned char pcr_irql(void)
{
    unsigned char irql;
    __asm__ volatile("movb %%fs:0x24, %0" : "=q"(irql));
    return irql;
}

static void __stdcall dpc_routine(PKDPC dpc, PVOID ctx, PVOID a1, PVOID a2)
{
    (void)dpc; (void)ctx; (void)a1; (void)a2;
    if (pcr_irql() != KeGetCurrentIrql() || pcr_irql() != DISPATCH_LEVEL)
        g_irql_ok = 0;
    if (g_use_sse)
        sse1_memcpy(g_dst, g_src, g_size);
    else
        memcpy(g_dst, g_src, g_size);  /* volatile size: a real libc call */
    g_dpc_count++;
}

/* What RXDK_FPU_SAFE_DPC generates for an SDK DPC. */
static void __stdcall dpc_routine_safe(PKDPC dpc, PVOID ctx, PVOID a1, PVOID a2)
{
    RxdkRunDpcFpuSafe((void *)dpc_routine, dpc, ctx, a1, a2);
}

/* Load xmm0-7 from in[], wait for `fires` more DPCs (or a spin limit), store
   xmm0-7 to out[]. Only integer code runs between the load and the store. */
static void hold_xmm(const unsigned int *in, unsigned int *out, LONG fires)
{
    LONG target = g_dpc_count + fires;
    unsigned long spins = 0x40000000UL;
    __asm__ volatile(
        "movups   0(%0), %%xmm0\n\t"
        "movups  16(%0), %%xmm1\n\t"
        "movups  32(%0), %%xmm2\n\t"
        "movups  48(%0), %%xmm3\n\t"
        "movups  64(%0), %%xmm4\n\t"
        "movups  80(%0), %%xmm5\n\t"
        "movups  96(%0), %%xmm6\n\t"
        "movups 112(%0), %%xmm7\n\t"
        "1:\n\t"
        "cmpl %3, %2\n\t"
        "jge 2f\n\t"
        "decl %4\n\t"
        "jnz 1b\n\t"
        "2:\n\t"
        "movups %%xmm0,   0(%1)\n\t"
        "movups %%xmm1,  16(%1)\n\t"
        "movups %%xmm2,  32(%1)\n\t"
        "movups %%xmm3,  48(%1)\n\t"
        "movups %%xmm4,  64(%1)\n\t"
        "movups %%xmm5,  80(%1)\n\t"
        "movups %%xmm6,  96(%1)\n\t"
        "movups %%xmm7, 112(%1)\n\t"
        : "+r"(in), "+r"(out), "+m"(g_dpc_count), "+r"(target), "+r"(spins)
        :
        : "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
}

/* Run DPCs copying `size` bytes while the thread holds xmm; returns the
   number of the 32 xmm dwords that changed, or -1 if too few DPCs ran. */
static int xmm_changed(PKDPC dpc, int use_sse, size_t size)
{
    unsigned int pattern[32], got[32];
    LARGE_INTEGER due;
    LONG before;
    int bad = 0, i;

    for (i = 0; i < 32; i++)
        pattern[i] = 0x5EED0000u + (unsigned)i * 0x01010101u;
    memset(got, 0, sizeof got);
    g_size = size;
    g_use_sse = use_sse;
    before = g_dpc_count;
    due.QuadPart = -10000;                       /* 1 ms, relative */
    KeSetTimerEx(&g_timer, due, 1, dpc);         /* then every 1 ms */
    hold_xmm(pattern, got, 50);
    KeCancelTimer(&g_timer);
    if (g_dpc_count - before < 50)
        return -1;
    for (i = 0; i < 32; i++)
        if (got[i] != pattern[i])
            bad++;
    return bad;
}

int main(void)
{
    static const size_t sizes[] = { 3, 7, 16, 48, 64, 600, 8192 };
    unsigned i;
    KIRQL old;

    for (i = 0; i < sizeof g_src; i++)
        g_src[i] = (unsigned char)(i * 7 + 3);

    /* memcpy reads the IRQL from the processor control region. */
    CHECK_EQI(pcr_irql(), KeGetCurrentIrql(), "fs:[0x24] is KeGetCurrentIrql at PASSIVE_LEVEL");
    CHECK_EQI(pcr_irql(), PASSIVE_LEVEL, "a title thread runs at PASSIVE_LEVEL");
    old = KeRaiseIrqlToDpcLevel();
    {
        unsigned char raised = pcr_irql();
        KIRQL k = KeGetCurrentIrql();
        KfLowerIrql(old);
        CHECK_EQI(raised, k, "fs:[0x24] is KeGetCurrentIrql at DISPATCH_LEVEL");
        CHECK_EQI(raised, DISPATCH_LEVEL, "KeRaiseIrqlToDpcLevel raises to DISPATCH_LEVEL");
    }

    KeInitializeDpc(&g_dpc, (PKDEFERRED_ROUTINE)dpc_routine, NULL);
    KeInitializeDpc(&g_dpc_safe, (PKDEFERRED_ROUTINE)dpc_routine_safe, NULL);
    KeInitializeTimerEx(&g_timer, SynchronizationTimer);
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        char desc[96];
        snprintf(desc, sizeof desc, "xmm0-7 intact across DPCs doing memcpy(%u)", (unsigned)sizes[i]);
        CHECK_EQI(xmm_changed(&g_dpc, 0, sizes[i]), 0, desc);
    }
    CHECK(xmm_changed(&g_dpc, 1, 48) > 0,
          "a DPC that uses SSE unprotected does change the interrupted thread's xmm");
    CHECK_EQI(xmm_changed(&g_dpc_safe, 1, 48), 0,
              "xmm0-7 intact across DPCs using SSE through RxdkRunDpcFpuSafe");
    CHECK(g_irql_ok, "fs:[0x24] is KeGetCurrentIrql (DISPATCH_LEVEL) inside a DPC");
    CHECK(memcmp(g_dst, g_src, sizeof g_dst) == 0, "the DPC's integer memcpy copied the bytes");

    CHECK_DONE("dpc_fpu");
    return 0;
}
