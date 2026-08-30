/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Finding under test: MSVC printf conversions through the *bounded* narrow family.
 *
 * RXDK translated MSVC's %S/%C/%I64 for sprintf and the _snprintf family, but the
 * standard snprintf/vsnprintf were picolibc's untranslated versions -- so portable
 * code (e.g. SDL's SDL_vsnprintf when HAVE_VSNPRINTF is set) got the literal text
 * "%S" instead of the wide string. This title verifies snprintf/vsnprintf now apply
 * the same translation, and that plain conversions and the C99 return value are
 * unaffected. Results via DbgPrint (read under xbWatson).
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

extern void DbgPrint(const char *format, ...);

static int g_pass = 0;
static int g_fail = 0;

static void check(int cond, const char *name, const char *got)
{
    if (cond) {
        DbgPrint("  PASS  %s\n", name);
        g_pass++;
    } else {
        DbgPrint("  FAIL  %s  got=\"%s\"\n", name, got);
        g_fail++;
    }
}

/* Reaches snprintf's sibling vsnprintf through a va_list, as SDL does. */
static void vfmt(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, n, fmt, ap);
    va_end(ap);
}

int main(void)
{
    char buf[64];
    int need;

    DbgPrint("RXDK tests: StringFormat start\n");

    /* %S in a narrow snprintf is MSVC's "wide string" -- the SDL papercut. */
    snprintf(buf, sizeof buf, "d:\\%S", L"Media\\");
    check(strcmp(buf, "d:\\Media\\") == 0, "snprintf %S expands a wide string", buf);

    /* Same conversion, but reached through vsnprintf (SDL_vsnprintf's path). */
    vfmt(buf, sizeof buf, "%S", L"wide");
    check(strcmp(buf, "wide") == 0, "vsnprintf %S expands a wide string", buf);

    /* %C is MSVC's wide char. */
    snprintf(buf, sizeof buf, "[%C]", L'X');
    check(strcmp(buf, "[X]") == 0, "snprintf %C expands a wide char", buf);

    /* %I64d is MSVC's 64-bit integer spelling. */
    snprintf(buf, sizeof buf, "%I64d", 1234567890123LL);
    check(strcmp(buf, "1234567890123") == 0, "snprintf %I64d formats 64-bit", buf);

    /* Plain narrow conversions must be untouched. */
    snprintf(buf, sizeof buf, "%s/%d", "narrow", 7);
    check(strcmp(buf, "narrow/7") == 0, "snprintf plain %s/%d unaffected", buf);

    /* C99 return value (the length it would have needed) is preserved. */
    need = snprintf(buf, sizeof buf, "%d", 42);
    check(need == 2, "snprintf returns C99 length", buf);

    DbgPrint("RXDK tests: StringFormat done  pass=%d fail=%d\n", g_pass, g_fail);

    for (;;)
        ;
    return 0;
}
