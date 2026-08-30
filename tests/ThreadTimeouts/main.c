/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Finding under test: C11 <threads.h> timed primitives.
 *
 * mtx_timedlock and cnd_timedwait used to ignore their deadline and block
 * forever while reporting success. This title verifies they now actually time
 * out (returning thrd_timedout) and still succeed on the happy path. Results are
 * printed via DbgPrint, so run under xbWatson / the debug monitor to read them.
 */

#include <threads.h>
#include <time.h>

/* Kernel debug-console sink (libkernel import); declared here so the test needs
   no internal headers. Varargs => cdecl, which matches the kernel export. */
extern void DbgPrint(const char *format, ...);

static int g_pass = 0;
static int g_fail = 0;

static void check(int cond, const char *name)
{
    if (cond) {
        DbgPrint("  PASS  %s\n", name);
        g_pass++;
    } else {
        DbgPrint("  FAIL  %s\n", name);
        g_fail++;
    }
}

/* An absolute TIME_UTC deadline `ms` milliseconds from now (what the C11 timed
   calls expect). */
static struct timespec deadline_ms(long ms)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return ts;
}

static void sleep_ms(long ms)
{
    struct timespec d;
    d.tv_sec = ms / 1000;
    d.tv_nsec = (ms % 1000) * 1000000L;
    thrd_sleep(&d, NULL);
}

/* Worker that grabs g_mtx and holds it, so the main thread's timed lock has to
   contend with a real owner (a recursive re-lock on the same thread would not). */
static mtx_t g_mtx;

static int hold_worker(void *arg)
{
    (void)arg;
    mtx_lock(&g_mtx);
    sleep_ms(500);
    mtx_unlock(&g_mtx);
    return 0;
}

int main(void)
{
    thrd_t worker;
    struct timespec ts;
    int rc;

    DbgPrint("RXDK tests: ThreadTimeouts start\n");

    /* 1) mtx_timedlock happy path: a free mutex is acquired at once. */
    mtx_init(&g_mtx, mtx_plain);
    ts = deadline_ms(100);
    rc = mtx_timedlock(&g_mtx, &ts);
    check(rc == thrd_success, "mtx_timedlock acquires a free mutex");
    if (rc == thrd_success)
        mtx_unlock(&g_mtx);

    /* 2) mtx_timedlock timeout: worker holds it ~500ms; our 100ms deadline must
          expire with thrd_timedout (not block forever, not falsely succeed). */
    thrd_create(&worker, hold_worker, NULL);
    sleep_ms(50); /* let the worker take the lock first */
    ts = deadline_ms(100);
    rc = mtx_timedlock(&g_mtx, &ts);
    check(rc == thrd_timedout, "mtx_timedlock times out on a held mutex");
    if (rc == thrd_success)
        mtx_unlock(&g_mtx);
    thrd_join(worker, NULL);

    /* 3) cnd_timedwait timeout: nobody signals, so a 150ms deadline must expire
          with thrd_timedout and the mutex must come back locked. */
    {
        cnd_t cnd;
        mtx_t m;
        cnd_init(&cnd);
        mtx_init(&m, mtx_plain);
        mtx_lock(&m);
        ts = deadline_ms(150);
        rc = cnd_timedwait(&cnd, &m, &ts);
        check(rc == thrd_timedout, "cnd_timedwait times out without a signal");
        mtx_unlock(&m);
    }

    DbgPrint("RXDK tests: ThreadTimeouts done  pass=%d fail=%d\n", g_pass, g_fail);

    for (;;)
        sleep_ms(1000);
    return 0;
}
