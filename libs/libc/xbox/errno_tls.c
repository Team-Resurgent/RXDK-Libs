/*
 * Per-thread errno for picolibc.
 *
 * Selected by __PICOLIBC_ERRNO_FUNCTION=__rxdk_errno (build/generated/picolibc.h),
 * which makes <errno.h> expand `errno` to `(*__rxdk_errno())`. Each thread gets
 * its own int, keyed by KeGetCurrentThread() in a flat table. The returned
 * pointer is stable once allocated, so reads/writes after the first access are
 * lock-free; only slot allocation takes the lock. Kernel-only; no libxapi.
 */

#include "xbox/kernel.h"

#define RXDK_ERRNO_SLOTS 64

static struct {
    PKTHREAD kt;
    int used;
    int err;
} g_slots[RXDK_ERRNO_SLOTS];

static int g_fallback;
static RTL_CRITICAL_SECTION g_lock;
static int g_lock_init; /* first errno access is single-threaded (main) */

int *__rxdk_errno(void)
{
    PKTHREAD kt = KeGetCurrentThread();
    int *result = NULL;

    if (!g_lock_init) {
        RtlInitializeCriticalSection(&g_lock);
        g_lock_init = 1;
    }

    RtlEnterCriticalSection(&g_lock);
    for (int i = 0; i < RXDK_ERRNO_SLOTS; ++i) {
        if (g_slots[i].used && g_slots[i].kt == kt) {
            result = &g_slots[i].err;
            break;
        }
    }
    if (!result) {
        for (int i = 0; i < RXDK_ERRNO_SLOTS; ++i) {
            if (!g_slots[i].used) {
                g_slots[i].used = 1;
                g_slots[i].kt = kt;
                g_slots[i].err = 0;
                result = &g_slots[i].err;
                break;
            }
        }
    }
    RtlLeaveCriticalSection(&g_lock);

    return result ? result : &g_fallback;
}
