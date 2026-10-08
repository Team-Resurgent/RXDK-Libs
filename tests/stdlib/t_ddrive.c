/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* D: is the directory the running XBE was launched from -- the disc root for a
   disc title, the title's folder when a dashboard starts it from the hard disk --
   so a title always finds the files shipped next to its default.xbe.
   Original Xbox only (the 360 uses game:). */
#include "rxdk_test.h"
#include <string.h>
#include <sys/stat.h>

/* As <xboxkrnl/api/xc.h> declares it (that header's DbgPrint prototype clashes
   with rxdk_test.h's, so it is not included). */
typedef struct {
    unsigned short Length;
    unsigned short MaximumLength;
    char          *Buffer;
} t_ansi_string;
extern __declspec(dllimport) t_ansi_string XeImageFileName[1];

int main(void)
{
    const t_ansi_string *image = &XeImageFileName[0];
    char               path[300];
    char               name[260];
    struct stat        st;

    CHECK(image->Buffer != NULL && image->Length > 0, "XeImageFileName is set");
    CHECK(image->Length > 8 && strncmp(image->Buffer, "\\Device\\", 8) == 0,
          "XeImageFileName is an NT device path");

    /* The XBE's own file name, after the last separator. */
    unsigned short end = image->Length;
    unsigned short start = end;
    while (start > 0 && image->Buffer[start - 1] != '\\')
        start--;
    unsigned short len = (unsigned short)(end - start);
    CHECK(len > 0 && len < sizeof name, "XeImageFileName has a file name");
    if (len == 0 || len >= sizeof name)
        len = 0;
    memcpy(name, image->Buffer + start, len);
    name[len] = '\0';

    strcpy(path, "D:\\");
    strcat(path, name);
    CHECK_EQI(stat(path, &st), 0, "the running XBE is at D:\\<its name>");
    CHECK(S_ISREG(st.st_mode) && st.st_size > 0, "D:\\<xbe> is a non-empty file");

    CHECK_DONE("ddrive");
    return 0;
}
