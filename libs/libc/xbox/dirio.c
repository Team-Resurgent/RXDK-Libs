/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Directory enumeration + extended path operations for picolibc, on the Xbox
 * kernel. This is the POSIX surface <filesystem> (libc++) needs beyond the
 * basic file I/O in fileio.c: <dirent.h> (opendir/readdir/...) over
 * NtQueryDirectoryFile, plus rename/truncate/getcwd/chdir/statvfs/utimes/
 * realpath/lstat/openat/unlinkat/fchmod/fchmodat/pathconf. Symlinks and hard
 * links do not exist on FATX, so those report ENOSYS/EINVAL honestly.
 *
 * Kernel-only (Nt, Rtl, Ke, Ob exports); never depends on libxapi. The fd-based
 * ops share fileio.c's descriptor table via __rxdk_fd_handle/__rxdk_fd_install.
 *
 * Note: rename() is a C-standard <stdio.h> function (picolibc ships no generic
 * body), not POSIX — it lives here because it shares the NT path machinery.
 */

/* Unlock POSIX.1-2008 declarations (openat family, AT_* flags, statvfs,
   readlink, ...) that -std=c23 hides behind __POSIX_VISIBLE. */
#define _GNU_SOURCE 1

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <unistd.h>

#include "xbox/kernel.h"

/* --- NT ABI constants (mirrors the private set in fileio.c). */
#define NT_GENERIC_READ        0x80000000UL
#define NT_GENERIC_WRITE       0x40000000UL
#define NT_SYNCHRONIZE         0x00100000UL
#define NT_DELETE              0x00010000UL
#define NT_FILE_READ_ATTRS     0x00000080UL
#define NT_FILE_WRITE_ATTRS    0x00000100UL
#define NT_FILE_LIST_DIRECTORY 0x00000001UL

#define NT_FILE_SHARE_READ     0x00000001UL
#define NT_FILE_SHARE_WRITE    0x00000002UL
#define NT_FILE_SHARE_DELETE   0x00000004UL

#define NT_FILE_OPEN           1UL
#define NT_FILE_CREATE         2UL
#define NT_FILE_OPEN_IF        3UL

#define NT_FILE_DIRECTORY_FILE          0x00000001UL
#define NT_FILE_SYNCHRONOUS_IO_NONALERT 0x00000020UL
#define NT_FILE_NON_DIRECTORY_FILE      0x00000040UL

#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL  0x00000080UL
#endif

/* 1601->1970 epoch difference, in 100ns units. */
#define RXDK_EPOCH_DIFF_100NS 116444736000000000LL

#ifndef O_DIRECTORY
#define O_DIRECTORY 0x200000
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

extern HANDLE __rxdk_fd_handle(int fd);
extern int __rxdk_fd_install(HANDLE h);
extern int __rxdk_fd_install_path(HANDLE h, const char *path);
extern const char *__rxdk_fd_path(int fd);

/* Current working directory (no per-process cwd on Xbox; libc tracks one).
   Defaults to D:\ -- the title's own home directory (where default.xbe and its
   bundled files live), which is what portable code calling fopen("assets/x") /
   stat() with a relative path expects. A title can chdir() elsewhere (e.g. a
   writable data partition) if it wants a different base. */
static char g_cwd[PATH_MAX] = "D:\\";

static void to_backslash(char *s)
{
    for (; *s; ++s)
        if (*s == '/')
            *s = '\\';
}

/* Collapse '.', '..' and empty ("\\\\") segments of an absolute DOS path in
   place (e.g. "T:\\a\\.\\b\\..\\c" -> "T:\\a\\c"). Keeps the "X:" drive prefix;
   a fully-popped path becomes the drive root "X:\\". FATX has no notion of '.'
   or '..' entries, so paths must be pre-collapsed before they reach the object
   manager (which would otherwise reject or mis-open them). */
static void collapse_dots(char *path)
{
    char *segs[PATH_MAX / 2];
    int nseg = 0;
    char *body;
    char work[PATH_MAX];
    char *p, *save;

    if (!(path[0] && path[1] == ':'))
        return; /* not drive-absolute; leave as-is */

    body = path + 2;               /* after "X:" */
    while (*body == '\\')
        ++body;                    /* skip leading separators */

    /* Tokenize a scratch copy so the segment pointers stay valid. */
    strncpy(work, body, sizeof work - 1);
    work[sizeof work - 1] = '\0';

    for (p = work; ; p = NULL) {
        char *tok = strtok_r(p, "\\", &save);
        if (!tok)
            break;
        if (tok[0] == '.' && tok[1] == '\0')
            continue;              /* '.' -> drop */
        if (tok[0] == '.' && tok[1] == '.' && tok[2] == '\0') {
            if (nseg > 0)
                --nseg;            /* '..' -> pop */
            continue;
        }
        if (nseg < (int)(sizeof segs / sizeof segs[0]))
            segs[nseg++] = tok;
    }

    /* Rebuild: "X:\\seg1\\seg2..." or the drive root when empty. */
    p = path + 2;
    *p++ = '\\';
    for (int i = 0; i < nseg; ++i) {
        size_t len = strlen(segs[i]);
        if (i > 0)
            *p++ = '\\';
        memcpy(p, segs[i], len);
        p += len;
    }
    *p = '\0';
}

/*
 * Normalize an incoming path to a DOS-style absolute name ("E:\\dir\\file")
 * suitable for the object manager. libc++ runs in POSIX mode (no _WIN32), so
 * it may hand us '/' separators; drive-absolute paths are taken as-is, rooted
 * paths inherit the cwd drive, and relative paths are resolved against cwd.
 */
const char *__rxdk_norm_path(const char *in, char *out, size_t outsz)
{
    if (!in) {
        errno = EINVAL;
        return NULL;
    }
    if (in[0] && in[1] == ':') { /* drive-absolute */
        strncpy(out, in, outsz - 1);
        out[outsz - 1] = '\0';
    } else if (in[0] == '/' || in[0] == '\\') { /* rooted, no drive */
        out[0] = g_cwd[0];
        out[1] = g_cwd[1];
        out[2] = '\0';
        strncat(out, in, outsz - 3);
    } else { /* relative */
        size_t cl = strlen(g_cwd);
        /* Don't double the separator when the cwd already ends with one (the
           drive root is "D:\\"); NT's object manager won't collapse "D:\\\\x". */
        if (cl > 0 && (g_cwd[cl - 1] == '\\' || g_cwd[cl - 1] == '/')) {
            snprintf(out, outsz, "%s%s", g_cwd, in);
        } else {
            snprintf(out, outsz, "%s\\%s", g_cwd, in);
        }
    }
    to_backslash(out);
    collapse_dots(out);
    return out;
}

static int resolve_at(int dirfd, const char *path, char *out, size_t outsz);

/* Map an NTSTATUS to errno. Distinguishing "doesn't exist" from other failures
   matters: std::filesystem::remove_all keys off ENOENT vs ENOTDIR. */
static int errno_from_status(NTSTATUS s)
{
    switch ((unsigned long)s) {
    case 0xC0000034UL: /* STATUS_OBJECT_NAME_NOT_FOUND */
    case 0xC000003AUL: /* STATUS_OBJECT_PATH_NOT_FOUND */
        return ENOENT;
    case 0xC0000035UL: /* STATUS_OBJECT_NAME_COLLISION */
        return EEXIST;
    case 0xC0000022UL: /* STATUS_ACCESS_DENIED */
        return EACCES;
    case 0xC0000103UL: /* STATUS_NOT_A_DIRECTORY */
        return ENOTDIR;
    case 0xC0000101UL: /* STATUS_DIRECTORY_NOT_EMPTY */
        return ENOTEMPTY;
    default:
        return EIO;
    }
}

static NTSTATUS path_open(HANDLE root, const char *name, ULONG access,
                          ULONG disp, ULONG options, HANDLE *out)
{
    OBJECT_STRING s;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;

    RtlInitAnsiString(&s, name);
    InitializeObjectAttributes(&oa, &s, OBJ_CASE_INSENSITIVE, root, NULL);
    return NtCreateFile(out, access | NT_SYNCHRONIZE, &oa, &iosb, NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        NT_FILE_SHARE_READ | NT_FILE_SHARE_WRITE |
                            NT_FILE_SHARE_DELETE,
                        disp, options | NT_FILE_SYNCHRONOUS_IO_NONALERT);
}

/* ---- directory enumeration (<dirent.h>) ----------------------------------- */

DIR *opendir(const char *path)
{
    char np[PATH_MAX];
    HANDLE h;
    NTSTATUS s;
    DIR *d;
    int fd;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return NULL;
    s = path_open(ObDosDevicesDirectory(), np,
                  NT_GENERIC_READ | NT_FILE_LIST_DIRECTORY, NT_FILE_OPEN,
                  NT_FILE_DIRECTORY_FILE, &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return NULL;
    }
    fd = __rxdk_fd_install_path(h, np); /* remember path for *at() resolution */
    if (fd < 0) {
        NtClose(h);
        errno = EMFILE;
        return NULL;
    }
    d = (DIR *)malloc(sizeof(DIR));
    if (!d) {
        close(fd);
        errno = ENOMEM;
        return NULL;
    }
    d->fd = fd;
    d->offset = 0;
    d->count = 0;
    d->pos = 0;
    d->restart = 1;
    return d;
}

DIR *fdopendir(int fd)
{
    DIR *d;
    if (!__rxdk_fd_handle(fd)) {
        errno = EBADF;
        return NULL;
    }
    d = (DIR *)malloc(sizeof(DIR));
    if (!d) {
        errno = ENOMEM;
        return NULL;
    }
    d->fd = fd; /* fdopendir takes ownership; closedir closes it */
    d->offset = 0;
    d->count = 0;
    d->pos = 0;
    d->restart = 1;
    return d;
}

struct dirent *readdir(DIR *d)
{
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS s;
    FILE_DIRECTORY_INFORMATION *fdi;
    ULONG nlen;
    size_t cap = sizeof(d->dirent.d_name) - 1;

    if (!d) {
        errno = EBADF;
        return NULL;
    }
    h = __rxdk_fd_handle(d->fd);
    if (!h) {
        errno = EBADF;
        return NULL;
    }

    if (d->offset >= d->count) { /* batch exhausted -> fetch the next one */
        /* RestartScan on the first fetch after opendir/rewinddir/seekdir so the
           kernel enumerates from the top (telldir/seekdir rely on this). */
        s = NtQueryDirectoryFile(h, NULL, NULL, NULL, &iosb, d->buf,
                                 sizeof(d->buf), FileDirectoryInformation,
                                 NULL, d->restart ? TRUE : FALSE);
        d->restart = 0;
        if (!NT_SUCCESS(s)) /* STATUS_NO_MORE_FILES or error -> end of stream */
            return NULL;
        d->count = (size_t)iosb.Information;
        d->offset = 0;
        if (d->count == 0)
            return NULL;
    }

    fdi = (FILE_DIRECTORY_INFORMATION *)(d->buf + d->offset);
    nlen = fdi->FileNameLength; /* bytes; FATX names are ANSI (OCHAR == char) */
    if (nlen > cap)
        nlen = (ULONG)cap;
    memcpy(d->dirent.d_name, fdi->FileName, nlen);
    d->dirent.d_name[nlen] = '\0';
    d->dirent.d_ino = 0;
    d->dirent.d_type =
        (fdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;

    if (fdi->NextEntryOffset)
        d->offset += fdi->NextEntryOffset;
    else
        d->offset = d->count; /* last in batch -> requery next call */
    d->pos++;                 /* advance the telldir index past this entry */
    return &d->dirent;
}

int closedir(DIR *d)
{
    if (!d) {
        errno = EBADF;
        return -1;
    }
    close(d->fd); /* closes the kernel handle + frees the ofd */
    free(d);
    return 0;
}

int dirfd(DIR *d)
{
    if (!d) {
        errno = EINVAL;
        return -1;
    }
    return d->fd;
}

void rewinddir(DIR *d)
{
    if (d) {
        d->offset = 0;
        d->count = 0;
        d->pos = 0;
        d->restart = 1; /* next readdir re-enumerates from the top */
    }
}

/* telldir returns the index of the entry readdir will return next; seekdir
   restores it by rewinding and re-reading (our enumeration has no cheaper
   absolute seek, but this is exact). */
long telldir(DIR *d)
{
    if (!d) { errno = EBADF; return -1; }
    return d->pos;
}

void seekdir(DIR *d, long loc)
{
    if (!d || loc < 0)
        return;
    rewinddir(d);
    while (d->pos < loc && readdir(d) != NULL)
        ; /* readdir advances d->pos */
}

/* faccessat: only AT_FDCWD is supported (FATX has no dir-relative open); the
   flag (AT_EACCESS) is irrelevant with no per-file permissions. */
int faccessat(int dirfd, const char *path, int amode, int flag)
{
    (void)flag;
    if (dirfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return access(path, amode);
}

int mkdirat(int dirfd, const char *path, mode_t mode)
{
    if (dirfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return mkdir(path, mode);
}

int fstatat(int dirfd, const char *path, struct stat *st, int flag)
{
    (void)flag;
    if (dirfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return stat(path, st);
}

/* ---- remove / rename (C <stdio.h>) ----------------------------------------- */

/* POSIX remove(): unlink for files, rmdir for directories. picolibc ships an
   unlink-only remove(), which can't delete directories (std::filesystem::remove
   relies on the directory-aware behavior). */
int remove(const char *path)
{
    char np[PATH_MAX];
    struct stat st;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return -1;
    if (stat(np, &st) == 0 && S_ISDIR(st.st_mode))
        return rmdir(np);
    return unlink(np);
}

int rename(const char *oldp, const char *newp)
{
    char no[PATH_MAX];
    char nn[PATH_MAX];
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_RENAME_INFORMATION ri;
    NTSTATUS s;

    if (!__rxdk_norm_path(oldp, no, sizeof no) || !__rxdk_norm_path(newp, nn, sizeof nn))
        return -1;
    s = path_open(ObDosDevicesDirectory(), no, NT_DELETE, NT_FILE_OPEN, 0, &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return -1;
    }
    ri.ReplaceIfExists = TRUE;
    ri.RootDirectory = ObDosDevicesDirectory();
    RtlInitAnsiString(&ri.FileName, nn);
    s = NtSetInformationFile(h, &iosb, &ri, sizeof ri, FileRenameInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

/* ---- truncate -------------------------------------------------------------- */

int ftruncate(int fd, off_t length)
{
    HANDLE h = __rxdk_fd_handle(fd);
    IO_STATUS_BLOCK iosb;
    FILE_END_OF_FILE_INFORMATION eof;
    NTSTATUS s;

    if (!h) {
        errno = EBADF;
        return -1;
    }
    eof.EndOfFile.QuadPart = (long long)length;
    s = NtSetInformationFile(h, &iosb, &eof, sizeof eof,
                             FileEndOfFileInformation);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

int truncate(const char *path, off_t length)
{
    char np[PATH_MAX];
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_END_OF_FILE_INFORMATION eof;
    NTSTATUS s;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return -1;
    s = path_open(ObDosDevicesDirectory(), np, NT_GENERIC_WRITE, NT_FILE_OPEN,
                  NT_FILE_NON_DIRECTORY_FILE, &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return -1;
    }
    eof.EndOfFile.QuadPart = (long long)length;
    s = NtSetInformationFile(h, &iosb, &eof, sizeof eof,
                             FileEndOfFileInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

/* ---- cwd ------------------------------------------------------------------- */

char *getcwd(char *buf, size_t size)
{
    size_t n = strlen(g_cwd);
    if (!buf) {
        errno = EINVAL;
        return NULL;
    }
    if (size < n + 1) {
        errno = ERANGE;
        return NULL;
    }
    memcpy(buf, g_cwd, n + 1);
    return buf;
}

int chdir(const char *path)
{
    char np[PATH_MAX];
    struct stat st;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return -1;
    if (stat(np, &st) != 0)
        return -1; /* errno from stat */
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    strncpy(g_cwd, np, sizeof g_cwd - 1);
    g_cwd[sizeof g_cwd - 1] = '\0';
    return 0;
}

/* ---- stat variants / realpath --------------------------------------------- */

int lstat(const char *path, struct stat *st)
{
    char np[PATH_MAX]; /* no symlinks on FATX -> lstat == stat */
    if (!__rxdk_norm_path(path, np, sizeof np))
        return -1;
    return stat(np, st);
}

char *realpath(const char *path, char *resolved)
{
    char np[PATH_MAX];
    char *out = resolved;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return NULL;
    if (!out) {
        out = (char *)malloc(PATH_MAX);
        if (!out) {
            errno = ENOMEM;
            return NULL;
        }
    }
    /* POSIX: `resolved` is a caller buffer of at least PATH_MAX bytes, but it may
       be exactly the resolved length + 1 (t_cwd passes char[256]). Copy only the
       string itself -- strncpy(out, np, PATH_MAX-1) would null-PAD to 1023 bytes
       and smash a caller buffer smaller than PATH_MAX. */
    {
        size_t len = strlen(np);
        if (len >= PATH_MAX)
            len = PATH_MAX - 1;
        memcpy(out, np, len);
        out[len] = '\0';
    }
    return out;
}

/* ---- volume info ----------------------------------------------------------- */

int statvfs(const char *path, struct statvfs *buf)
{
    char np[PATH_MAX];
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_FS_SIZE_INFORMATION si;
    NTSTATUS s;
    unsigned long unit;

    if (!buf || !__rxdk_norm_path(path, np, sizeof np)) {
        errno = EINVAL;
        return -1;
    }
    s = path_open(ObDosDevicesDirectory(), np, NT_GENERIC_READ, NT_FILE_OPEN, 0,
                  &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return -1;
    }
    s = NtQueryVolumeInformationFile(h, &iosb, &si, sizeof si,
                                     FileFsSizeInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    unit = (unsigned long)si.SectorsPerAllocationUnit * si.BytesPerSector;
    memset(buf, 0, sizeof *buf);
    buf->f_bsize = unit;
    buf->f_frsize = unit;
    buf->f_blocks = (fsblkcnt_t)si.TotalAllocationUnits.QuadPart;
    buf->f_bfree = (fsblkcnt_t)si.AvailableAllocationUnits.QuadPart;
    buf->f_bavail = buf->f_bfree;
    buf->f_namemax = NAME_MAX;
    return 0;
}

int fstatvfs(int fd, struct statvfs *buf)
{
    HANDLE h = __rxdk_fd_handle(fd);
    IO_STATUS_BLOCK iosb;
    FILE_FS_SIZE_INFORMATION si;
    NTSTATUS s;
    unsigned long unit;

    if (!buf || !h) {
        errno = EBADF;
        return -1;
    }
    s = NtQueryVolumeInformationFile(h, &iosb, &si, sizeof si,
                                     FileFsSizeInformation);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    unit = (unsigned long)si.SectorsPerAllocationUnit * si.BytesPerSector;
    memset(buf, 0, sizeof *buf);
    buf->f_bsize = unit;
    buf->f_frsize = unit;
    buf->f_blocks = (fsblkcnt_t)si.TotalAllocationUnits.QuadPart;
    buf->f_bfree = (fsblkcnt_t)si.AvailableAllocationUnits.QuadPart;
    buf->f_bavail = buf->f_bfree;
    buf->f_namemax = NAME_MAX;
    return 0;
}

/* ---- timestamps ------------------------------------------------------------ */

static long long tv_to_nt(const struct timeval *tv)
{
    return (long long)tv->tv_sec * 10000000LL + (long long)tv->tv_usec * 10LL +
           RXDK_EPOCH_DIFF_100NS;
}

int utimes(const char *path, const struct timeval times[2])
{
    char np[PATH_MAX];
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_BASIC_INFORMATION bi;
    NTSTATUS s;

    if (!__rxdk_norm_path(path, np, sizeof np))
        return -1;
    s = path_open(ObDosDevicesDirectory(), np,
                  NT_FILE_READ_ATTRS | NT_FILE_WRITE_ATTRS, NT_FILE_OPEN, 0, &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return -1;
    }
    memset(&bi, 0, sizeof bi); /* 0 fields = leave unchanged */
    if (times) {
        bi.LastAccessTime.QuadPart = tv_to_nt(&times[0]);
        bi.LastWriteTime.QuadPart = tv_to_nt(&times[1]);
    } else {
        LARGE_INTEGER now;
        KeQuerySystemTime(&now);
        bi.LastAccessTime = now;
        bi.LastWriteTime = now;
    }
    s = NtSetInformationFile(h, &iosb, &bi, sizeof bi, FileBasicInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

/* ---- permissions (FATX: only the read-only attribute is meaningful) -------- */

static int set_readonly(HANDLE h, int readonly)
{
    IO_STATUS_BLOCK iosb;
    FILE_BASIC_INFORMATION bi;
    NTSTATUS s;

    s = NtQueryInformationFile(h, &iosb, &bi, sizeof bi, FileBasicInformation);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    /* don't disturb timestamps */
    bi.CreationTime.QuadPart = 0;
    bi.LastAccessTime.QuadPart = 0;
    bi.LastWriteTime.QuadPart = 0;
    bi.ChangeTime.QuadPart = 0;
    if (readonly)
        bi.FileAttributes |= FILE_ATTRIBUTE_READONLY;
    else
        bi.FileAttributes &= ~(ULONG)FILE_ATTRIBUTE_READONLY;
    if (bi.FileAttributes == 0)
        bi.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    s = NtSetInformationFile(h, &iosb, &bi, sizeof bi, FileBasicInformation);
    if (!NT_SUCCESS(s)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

int fchmod(int fd, mode_t mode)
{
    HANDLE h = __rxdk_fd_handle(fd);
    if (!h) {
        errno = EBADF;
        return -1;
    }
    return set_readonly(h, !(mode & S_IWUSR));
}

int fchmodat(int dirfd, const char *path, mode_t mode, int flag)
{
    char np[PATH_MAX];
    HANDLE h;
    NTSTATUS s;
    int r;

    (void)flag;
    if (resolve_at(dirfd, path, np, sizeof np) != 0)
        return -1;
    s = path_open(ObDosDevicesDirectory(), np,
                  NT_FILE_READ_ATTRS | NT_FILE_WRITE_ATTRS, NT_FILE_OPEN, 0, &h);
    if (!NT_SUCCESS(s)) {
        errno = ENOENT;
        return -1;
    }
    r = set_readonly(h, !(mode & S_IWUSR));
    NtClose(h);
    return r;
}

/* ---- *at openers (dirfd-relative; used by std::filesystem::remove_all) ----- */

/*
 * Resolve an *at() (dirfd, path) pair to an absolute DOS path in `out`. FATX
 * has no RootDirectory-relative open, so dirfd-relative children are joined to
 * the directory fd's tracked absolute path. Returns 0 on success, -1 + errno.
 */
static int resolve_at(int dirfd, const char *path, char *out, size_t outsz)
{
    if (dirfd == AT_FDCWD || (path[0] && path[1] == ':')) {
        return __rxdk_norm_path(path, out, outsz) ? 0 : -1;
    } else {
        const char *dp = __rxdk_fd_path(dirfd);
        if (!dp) {
            errno = EBADF;
            return -1;
        }
        snprintf(out, outsz, "%s\\%s", dp, path);
        to_backslash(out);
        return 0;
    }
}

int openat(int dirfd, const char *path, int flags, ...)
{
    char np[PATH_MAX];
    HANDLE h;
    ULONG access;
    ULONG disp;
    ULONG options;
    NTSTATUS s;
    int fd;

    if (resolve_at(dirfd, path, np, sizeof np) != 0)
        return -1;

    switch (flags & O_ACCMODE) {
    case O_WRONLY: access = NT_GENERIC_WRITE; break;
    case O_RDWR:   access = NT_GENERIC_READ | NT_GENERIC_WRITE; break;
    default:       access = NT_GENERIC_READ; break;
    }
    disp = (flags & O_CREAT) ? ((flags & O_EXCL) ? NT_FILE_CREATE
                                                 : NT_FILE_OPEN_IF)
                             : NT_FILE_OPEN;
    if (flags & O_DIRECTORY) {
        options = NT_FILE_DIRECTORY_FILE;
        access |= NT_FILE_LIST_DIRECTORY;
    } else {
        options = NT_FILE_NON_DIRECTORY_FILE;
    }

    s = path_open(ObDosDevicesDirectory(), np, access, disp, options, &h);
    if (!NT_SUCCESS(s)) {
        /* O_DIRECTORY on an existing non-directory must report ENOTDIR (not
           ENOENT) so callers like remove_all fall through to file removal. */
        if (flags & O_DIRECTORY) {
            struct stat st;
            if (stat(np, &st) == 0 && !S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                return -1;
            }
        }
        errno = errno_from_status(s);
        return -1;
    }
    /* track the absolute path so nested *at() recursion (remove_all) works */
    fd = __rxdk_fd_install_path(h, np);
    if (fd < 0) {
        NtClose(h);
        errno = EMFILE;
        return -1;
    }
    return fd;
}

int unlinkat(int dirfd, const char *path, int flag)
{
    char np[PATH_MAX];
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_DISPOSITION_INFORMATION dispose;
    ULONG options;
    NTSTATUS s;

    if (resolve_at(dirfd, path, np, sizeof np) != 0)
        return -1;
    options = (flag & AT_REMOVEDIR) ? NT_FILE_DIRECTORY_FILE
                                    : NT_FILE_NON_DIRECTORY_FILE;
    s = path_open(ObDosDevicesDirectory(), np, NT_DELETE, NT_FILE_OPEN, options,
                  &h);
    if (!NT_SUCCESS(s)) {
        errno = errno_from_status(s);
        return -1;
    }
    dispose.DeleteFile = TRUE;
    s = NtSetInformationFile(h, &iosb, &dispose, sizeof dispose,
                             FileDispositionInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) {
        errno = errno_from_status(s);
        return -1;
    }
    return 0;
}

/* ---- limits ---------------------------------------------------------------- */

long pathconf(const char *path, int name)
{
    (void)path;
    switch (name) {
    case _PC_NAME_MAX: return NAME_MAX;
    case _PC_PATH_MAX: return PATH_MAX;
    case _PC_LINK_MAX: return 1;
    default:           return -1;
    }
}

/* ---- links: FATX has neither symlinks nor hard links ----------------------- */

int symlink(const char *target, const char *linkpath)
{
    (void)target;
    (void)linkpath;
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char *path, char *buf, size_t bufsize)
{
    (void)path; /* nothing is a symlink -> EINVAL per POSIX */
    (void)buf;
    (void)bufsize;
    errno = EINVAL;
    return -1;
}

int link(const char *oldpath, const char *newpath)
{
    (void)oldpath;
    (void)newpath;
    errno = ENOSYS;
    return -1;
}
