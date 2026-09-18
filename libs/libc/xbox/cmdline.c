/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Command-line tokenizer for building argv[]. Copies the (possibly kernel-
 * supplied) command line into a caller buffer and splits it on whitespace, with
 * simple double-quote grouping. Ported from RXDK-360 runtime/xbox/crt_start.c;
 * pure C, no platform dependencies. The Xbox launch path usually hands the title
 * an empty command line, so main()'s argc is normally 0.
 */
#include <stddef.h>

int __rxdk_parse_cmdline(const char *cl, char *buf, int bufsz,
                         char **argv, int argmax)
{
    int argc = 0, n = 0;
    char *p;

    if (!cl) {
        if (argmax > 0)
            argv[0] = (char *)0;
        return 0;
    }
    while (cl[n] && n < bufsz - 1) {
        buf[n] = cl[n];
        ++n;
    }
    buf[n] = '\0';

    p = buf;
    while (*p && argc < argmax - 1) {
        while (*p == ' ' || *p == '\t')
            ++p;
        if (!*p)
            break;
        if (*p == '"') {
            argv[argc++] = ++p;
            while (*p && *p != '"')
                ++p;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t')
                ++p;
        }
        if (*p)
            *p++ = '\0';
    }
    argv[argc] = (char *)0;
    return argc;
}
