/*
 * Temporary files for picolibc, on the Xbox scratch drive Z:
 * (\Device\Harddisk0\Partition5). picolibc's tmpnam/tmpfile hardcode a bare
 * "TXXXXXX" (relative to the cwd) and ignore P_tmpdir, so we provide our own
 * that target Z:\. tmpfile() unlinks immediately after create, relying on the
 * delete-on-close semantics enabled by FILE_SHARE_DELETE (see fileio.c), so the
 * scratch file vanishes when the stream is closed.
 *
 * Z: must be mounted for these to succeed (the harness maps it, as it does E:).
 * Kernel-only; no dependency on libxapi.
 */

#define _GNU_SOURCE 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RXDK_TMPDIR     "Z:\\"
#define RXDK_TMPDIR_LEN 3

/* Build "Z:\\Txxxxxx" (10 chars + NUL) into out, which must hold >= 11 bytes
   (L_tmpnam is sized for this). Uniqueness from getentropy + a local counter. */
static void rxdk_tmpname(char *out)
{
    static const char hex[16] = "0123456789ABCDEF";
    static unsigned seq;
    unsigned long long r = 0;
    char *p;
    int i;

    if (getentropy(&r, sizeof r) != 0)
        r = 0;
    r ^= (unsigned long long)(seq++) * 0x9E3779B97F4A7C15ULL;

    memcpy(out, RXDK_TMPDIR, RXDK_TMPDIR_LEN);
    p = out + RXDK_TMPDIR_LEN;
    p[0] = 'T';
    for (i = 0; i < 6; ++i) {
        p[1 + i] = hex[r & 0xF];
        r >>= 4;
    }
    p[7] = '\0';
}

char *tmpnam(char *s)
{
    static char buf[L_tmpnam];
    if (!s)
        s = buf;
    rxdk_tmpname(s);
    return s;
}

FILE *tmpfile(void)
{
    char name[L_tmpnam];
    int fd = -1;
    int tries;
    FILE *f;

    /* names are random, so retry a few times in case of a collision */
    for (tries = 0; tries < 64; ++tries) {
        rxdk_tmpname(name);
        fd = open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd >= 0)
            break;
    }
    if (fd < 0) {
        errno = EIO;
        return NULL;
    }

    /* POSIX: the file has no lingering name; delete-on-close removes it when
       the stream is closed (the handle was opened share-delete). */
    unlink(name);

    f = fdopen(fd, "w+b");
    if (!f) {
        close(fd);
        return NULL;
    }
    return f;
}
