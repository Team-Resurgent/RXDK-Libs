/*
 * Time backend for picolibc on the Xbox kernel.
 *
 *   - wall clock  -> KeQuerySystemTime  (100ns ticks since 1601-01-01)
 *   - monotonic   -> KeQueryPerformanceCounter / KeQueryPerformanceFrequency
 *
 * picolibc routes time() through gettimeofday() and clock() through times();
 * we also override clock() directly off the performance counter. Kernel-only
 * dependencies, no libxapi.
 */

/* Expose the full clock API (clockid_t, CLOCK_MONOTONIC, clock_gettime).
   picolibc gates these behind __POSIX_VISIBLE/__GNU_VISIBLE, both off under
   -std=c23; CLOCK_MONOTONIC specifically needs __GNU_VISIBLE. */
#define _GNU_SOURCE 1

#include <errno.h>
#include <stddef.h>
#include <sys/time.h>
#include <time.h>

#include "xbox/kernel.h"

/* 100ns intervals between 1601-01-01 and 1970-01-01 (FILETIME -> Unix epoch). */
#define RXDK_EPOCH_DIFF_100NS 116444736000000000ULL
#define RXDK_100NS_PER_SEC    10000000ULL

static unsigned long long unix_100ns(void)
{
    LARGE_INTEGER t;
    KeQuerySystemTime(&t);
    unsigned long long v = (unsigned long long)t.QuadPart;
    return (v > RXDK_EPOCH_DIFF_100NS) ? (v - RXDK_EPOCH_DIFF_100NS) : 0;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (tv) {
        unsigned long long t = unix_100ns();
        tv->tv_sec = (time_t)(t / RXDK_100NS_PER_SEC);
        tv->tv_usec = (suseconds_t)((t % RXDK_100NS_PER_SEC) / 10ULL);
    }
    return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    if (!tp) { errno = EFAULT; return -1; }

    if (clk_id == CLOCK_MONOTONIC) {
        unsigned long long freq = KeQueryPerformanceFrequency();
        unsigned long long c = KeQueryPerformanceCounter();
        if (freq == 0) { tp->tv_sec = 0; tp->tv_nsec = 0; return 0; }
        tp->tv_sec = (time_t)(c / freq);
        tp->tv_nsec = (long)(((c % freq) * 1000000000ULL) / freq);
        return 0;
    }

    /* CLOCK_REALTIME and anything else: wall clock. */
    {
        unsigned long long t = unix_100ns();
        tp->tv_sec = (time_t)(t / RXDK_100NS_PER_SEC);
        tp->tv_nsec = (long)((t % RXDK_100NS_PER_SEC) * 100ULL);
    }
    return 0;
}

int clock_getres(clockid_t clk_id, struct timespec *tp)
{
    (void)clk_id;
    if (tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 100; /* KeQuerySystemTime granularity */
    }
    return 0;
}

clock_t clock(void)
{
    unsigned long long freq = KeQueryPerformanceFrequency();
    unsigned long long c;
    unsigned long long sec;
    unsigned long long rem;

    if (freq == 0)
        return (clock_t)-1;

    c = KeQueryPerformanceCounter();
    sec = c / freq;
    rem = c % freq;
    /* split to avoid 64-bit overflow on c * CLOCKS_PER_SEC */
    return (clock_t)(sec * (unsigned long long)CLOCKS_PER_SEC
                     + (rem * (unsigned long long)CLOCKS_PER_SEC) / freq);
}
