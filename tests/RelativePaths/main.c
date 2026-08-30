/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Finding under test: relative path resolution in plain libc.
 *
 * Portable code (SDL's RWopsSetUp, its bitmap tests, and countless ported games)
 * calls fopen()/stat() with a *relative* path and expects it rooted at the
 * title's own directory. On Xbox that directory is D:\. libc tracks a cwd and
 * resolves relative paths against it; this title verifies the default cwd is D:\
 * (so a bare fopen finds the title's files) and that relative paths follow cwd.
 *
 * Results via DbgPrint (read under xbWatson). Uses E:\ (a writable data drive)
 * to exercise the relative-write path without depending on D:\ being writable.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

int main(void)
{
    char cwd[260];
    struct stat st;
    FILE *fp;

    DbgPrint("RXDK tests: RelativePaths start\n");

    /* 1) The default cwd is the title's home directory, D:\ -- so a relative
          fopen("assets/x") lands next to default.xbe, as ported code expects. */
    cwd[0] = '\0';
    getcwd(cwd, sizeof cwd);
    DbgPrint("  cwd = %s\n", cwd);
    check(cwd[0] == 'D' && cwd[1] == ':', "default cwd is D:\\ (title directory)");

    /* 2) Relative paths follow cwd: move to a writable data drive, create a file
          by a bare relative name, and confirm it materialized at E:\<name>. */
    check(chdir("E:\\") == 0, "chdir(\"E:\\\") succeeds");
    cwd[0] = '\0';
    getcwd(cwd, sizeof cwd);
    check(cwd[0] == 'E' && cwd[1] == ':', "cwd updated to E:\\ after chdir");

    remove("E:\\rxdk_rel_probe.bin");
    fp = fopen("rxdk_rel_probe.bin", "wb"); /* relative -> E:\rxdk_rel_probe.bin */
    check(fp != NULL, "fopen(relative name) opens under cwd");
    if (fp) {
        fputs("hi", fp);
        fclose(fp);
    }
    check(stat("E:\\rxdk_rel_probe.bin", &st) == 0,
          "relative write resolved to E:\\<name>");
    remove("E:\\rxdk_rel_probe.bin");

    chdir("D:\\");
    DbgPrint("RXDK tests: RelativePaths done  pass=%d fail=%d\n", g_pass, g_fail);

    for (;;)
        ;
    return 0;
}
