/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Finding under test: C++ exceptions (DWARF/Itanium EH).
 *
 * The docs disagree on whether real EH works (README says yes; some build flags
 * and porting-notes say "no EH yet"). This title settles it on hardware: throw
 * across a frame, confirm a local object's destructor runs during unwinding,
 * catch by type, read what(), and rethrow. If it prints all PASS and reaches the
 * summary, EH genuinely works. Results go to DbgPrint (read under xbWatson).
 *
 * NOTE: this must be built with exceptions ENABLED. If the SDK's default title
 * flags are -fno-exceptions, either this fails to compile (a finding in itself)
 * or throws abort at the first throw -- which is exactly what we want to learn.
 */

#include <exception>
#include <stdexcept>

extern "C" void DbgPrint(const char *format, ...);

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char *name)
{
    if (cond) {
        DbgPrint("  PASS  %s\n", name);
        g_pass++;
    } else {
        DbgPrint("  FAIL  %s\n", name);
        g_fail++;
    }
}

/* Its destructor sets a flag, so we can prove the stack unwound through it. */
struct Guard {
    int *ran;
    ~Guard() { *ran = 1; }
};

static void thrower()
{
    throw std::runtime_error("boom");
}

int main()
{
    DbgPrint("RXDK tests: Exceptions start\n");

    /* 1) throw across a frame; the Guard destructor must run during unwind. */
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

    /* 2) rethrow preserves the in-flight exception. */
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

    DbgPrint("RXDK tests: Exceptions done  pass=%d fail=%d\n", g_pass, g_fail);

    for (;;)
        ;
    return 0;
}
