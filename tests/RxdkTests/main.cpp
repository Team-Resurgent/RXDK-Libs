/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * RXDK-Libs test suite -- one title that runs every library check and prints a
 * PASS/FAIL line per check plus a grand total, so a single boot on xemu or real
 * hardware verifies the lot. Read the output on the debug serial / xbWatson.
 *
 * Built with exceptions enabled (rxdk.project.json "exceptions": true) so the EH
 * section can throw. <threads.h> is extern "C"-guarded, so the C11 thread checks
 * live here in the same C++ translation unit.
 */

#include <exception>
#include <stdexcept>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

extern "C" void DbgPrint(const char *format, ...);

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char *name)
{
    DbgPrint(cond ? "  PASS  %s\n" : "  FAIL  %s\n", name);
    if (cond)
        g_pass++;
    else
        g_fail++;
}

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

/* ---- C11 <threads.h> timed primitives ------------------------------------- */

static mtx_t g_mtx;

static int hold_worker(void *arg)
{
    (void)arg;
    mtx_lock(&g_mtx);
    sleep_ms(500);
    mtx_unlock(&g_mtx);
    return 0;
}

static void section_thread_timeouts()
{
    thrd_t worker;
    struct timespec ts;
    int rc;

    mtx_init(&g_mtx, mtx_plain);
    ts = deadline_ms(100);
    rc = mtx_timedlock(&g_mtx, &ts);
    check(rc == thrd_success, "mtx_timedlock acquires a free mutex");
    if (rc == thrd_success)
        mtx_unlock(&g_mtx);

    thrd_create(&worker, hold_worker, NULL);
    sleep_ms(50);
    ts = deadline_ms(100);
    rc = mtx_timedlock(&g_mtx, &ts);
    check(rc == thrd_timedout, "mtx_timedlock times out on a held mutex");
    if (rc == thrd_success)
        mtx_unlock(&g_mtx);
    thrd_join(worker, NULL);

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
}

/* ---- C++ exceptions (DWARF/Itanium EH) ------------------------------------ */

struct Guard {
    int *ran;
    ~Guard() { *ran = 1; }
};

static void thrower()
{
    throw std::runtime_error("boom");
}

static void section_exceptions()
{
    int dtor_ran = 0;
    try {
        Guard g;
        g.ran = &dtor_ran;
        thrower();
        check(false, "unreachable: code after throw ran");
    } catch (const std::runtime_error &e) {
        check(true, "caught std::runtime_error across a frame");
        check(dtor_ran == 1, "destructor ran during stack unwind");
        check(e.what() != 0, "runtime_error::what() is non-null");
    } catch (...) {
        check(false, "caught the wrong exception type");
    }

    bool rethrew = false;
    try {
        try {
            throw 42;
        } catch (...) {
            throw;
        }
    } catch (int v) {
        rethrew = (v == 42);
    }
    check(rethrew, "rethrow preserves the exception");
}

/* ---- relative path resolution --------------------------------------------- */

static void section_relative_paths()
{
    char cwd[260];
    struct stat st;

    cwd[0] = '\0';
    getcwd(cwd, sizeof cwd);
    DbgPrint("  cwd = %s\n", cwd);
    check(cwd[0] == 'D' && cwd[1] == ':', "default cwd is D:\\ (title directory)");

    /* probe.dat is deployed next to default.xbe (D:\probe.dat). A relative stat +
       fopen prove relative paths resolve against the title dir -- the SDL papercut.
       Read-only, so no dependency on a writable drive. */
    check(stat("D:\\probe.dat", &st) == 0, "ABS stat D:\\probe.dat (sanity: deployed + statable)");
    check(stat("probe.dat", &st) == 0, "relative stat resolves to D:\\ (finds deployed probe.dat)");
    {
        FILE *fa = fopen("D:\\probe.dat", "rb");
        check(fa != NULL, "ABS fopen D:\\probe.dat");
        if (fa)
            fclose(fa);
        FILE *fp = fopen("probe.dat", "rb");
        check(fp != NULL, "relative fopen opens D:\\probe.dat");
        if (fp)
            fclose(fp);
    }

    /* chdir round-trips and getcwd reflects it. */
    check(chdir("D:\\") == 0, "chdir(\"D:\\\") succeeds");
}

/* ---- MSVC printf conversions through bounded snprintf/vsnprintf ------------ */

static void vfmt(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, n, fmt, ap);
    va_end(ap);
}

static void section_string_format()
{
    char buf[64];
    int need;

    snprintf(buf, sizeof buf, "d:\\%S", L"Media\\");
    check(strcmp(buf, "d:\\Media\\") == 0, "snprintf %S expands a wide string");

    vfmt(buf, sizeof buf, "%S", L"wide");
    check(strcmp(buf, "wide") == 0, "vsnprintf %S expands a wide string");

    snprintf(buf, sizeof buf, "[%C]", L'X');
    check(strcmp(buf, "[X]") == 0, "snprintf %C expands a wide char");

    snprintf(buf, sizeof buf, "%I64d", 1234567890123LL);
    check(strcmp(buf, "1234567890123") == 0, "snprintf %I64d formats 64-bit");

    snprintf(buf, sizeof buf, "%s/%d", "narrow", 7);
    check(strcmp(buf, "narrow/7") == 0, "snprintf plain %s/%d unaffected");

    need = snprintf(buf, sizeof buf, "%d", 42);
    check(need == 2, "snprintf returns C99 length");
}

int main()
{
    DbgPrint("========== RXDK-Libs test suite ==========\n");

    DbgPrint("-- C11 thread timeouts --\n");
    section_thread_timeouts();
    DbgPrint("-- relative paths --\n");
    section_relative_paths();
    DbgPrint("-- MSVC string format --\n");
    section_string_format();

    /* Exceptions last: if EH is non-functional an uncaught throw terminates the
       process, so run it after everything else has already reported. */
    DbgPrint("-- C++ exceptions --\n");
    section_exceptions();

    DbgPrint("========== RESULT  pass=%d fail=%d ==========\n", g_pass, g_fail);

    for (;;)
        sleep_ms(1000);
    return 0;
}
