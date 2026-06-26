/*
 * POSIX file I/O backend for picolibc, implemented directly on the Xbox kernel
 * (Nt* / Io* / Rtl* exports). This is the storage layer picolibc stdio calls
 * down into (fopen/fread/fwrite/fseek/...). It deliberately uses ONLY the
 * kernel + picolibc headers — never libxapi — so the libc -> kernel layering
 * stays acyclic (libxapi depends on libc, not the reverse).
 */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "xbox/kernel.h"

/* --- NT ABI constants not exposed by the slim kernel headers (fixed values). */
#define NT_GENERIC_READ                 0x80000000UL
#define NT_GENERIC_WRITE                0x40000000UL
#define NT_SYNCHRONIZE                  0x00100000UL
#define NT_FILE_READ_ATTRIBUTES         0x00000080UL
#define NT_DELETE                       0x00010000UL

#define NT_FILE_SHARE_READ              0x00000001UL
#define NT_FILE_SHARE_WRITE             0x00000002UL

#define NT_FILE_OPEN                    1UL
#define NT_FILE_CREATE                  2UL
#define NT_FILE_OPEN_IF                 3UL
#define NT_FILE_OVERWRITE               4UL
#define NT_FILE_OVERWRITE_IF            5UL

#define NT_FILE_DIRECTORY_FILE          0x00000001UL
#define NT_FILE_SYNCHRONOUS_IO_NONALERT 0x00000020UL
#define NT_FILE_NON_DIRECTORY_FILE      0x00000040UL

#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL           0x00000080UL
#endif

#define RXDK_FD_BASE 3
#define RXDK_FD_MAX  32

typedef struct {
    int used;
    int append;
    HANDLE handle;
    long long offset;
} rxdk_fd;

static rxdk_fd fd_table[RXDK_FD_MAX];

static int alloc_fd(HANDLE h)
{
    for (int i = RXDK_FD_BASE; i < RXDK_FD_MAX; ++i) {
        if (!fd_table[i].used) {
            fd_table[i].used = 1;
            fd_table[i].append = 0;
            fd_table[i].handle = h;
            fd_table[i].offset = 0;
            return i;
        }
    }
    return -1;
}

static rxdk_fd *get_fd(int fd)
{
    if (fd < RXDK_FD_BASE || fd >= RXDK_FD_MAX || !fd_table[fd].used)
        return NULL;
    return &fd_table[fd];
}

static long long file_size(HANDLE h)
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS st = NtQueryInformationFile(h, &iosb, &info, sizeof(info),
                                         FileStandardInformation);
    if (!NT_SUCCESS(st))
        return -1;
    return (long long)info.EndOfFile.QuadPart;
}

/* Open `path` (a DOS-style name like "E:\\dir\\file") under \??\. */
static NTSTATUS nt_open(const char *path, ULONG access, ULONG disp,
                        ULONG options, HANDLE *out)
{
    OBJECT_STRING name;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;

    RtlInitAnsiString(&name, path);
    InitializeObjectAttributes(&obja, &name, OBJ_CASE_INSENSITIVE,
                               ObDosDevicesDirectory(), NULL);
    return NtCreateFile(out,
                        access | NT_SYNCHRONIZE | NT_FILE_READ_ATTRIBUTES,
                        &obja, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
                        NT_FILE_SHARE_READ | NT_FILE_SHARE_WRITE,
                        disp,
                        options | NT_FILE_SYNCHRONOUS_IO_NONALERT);
}

int open(const char *path, int flags, ...)
{
    ULONG access;
    ULONG disp;
    HANDLE h;
    NTSTATUS st;
    int fd;

    switch (flags & O_ACCMODE) {
    case O_RDONLY: access = NT_GENERIC_READ; break;
    case O_WRONLY: access = NT_GENERIC_WRITE; break;
    default:       access = NT_GENERIC_READ | NT_GENERIC_WRITE; break;
    }

    if (flags & O_CREAT) {
        if (flags & O_EXCL)       disp = NT_FILE_CREATE;
        else if (flags & O_TRUNC) disp = NT_FILE_OVERWRITE_IF;
        else                      disp = NT_FILE_OPEN_IF;
    } else {
        disp = (flags & O_TRUNC) ? NT_FILE_OVERWRITE : NT_FILE_OPEN;
    }

    st = nt_open(path, access, disp, NT_FILE_NON_DIRECTORY_FILE, &h);
    if (!NT_SUCCESS(st)) {
        errno = ENOENT;
        return -1;
    }

    fd = alloc_fd(h);
    if (fd < 0) {
        NtClose(h);
        errno = EMFILE;
        return -1;
    }
    if (flags & O_APPEND) {
        long long sz = file_size(h);
        fd_table[fd].append = 1;
        fd_table[fd].offset = (sz > 0) ? sz : 0;
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count)
{
    rxdk_fd *e = get_fd(fd);
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    NTSTATUS st;

    if (!e) { errno = EBADF; return -1; }
    if (count == 0) return 0;

    off.QuadPart = e->offset;
    st = NtReadFile(e->handle, NULL, NULL, NULL, &iosb, buf, (ULONG)count, &off);
    if (st == STATUS_END_OF_FILE)
        return 0;
    if (!NT_SUCCESS(st)) { errno = EIO; return -1; }

    e->offset += (long long)iosb.Information;
    return (ssize_t)iosb.Information;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    rxdk_fd *e;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    NTSTATUS st;

    if (!buf || count == 0)
        return 0;

    /* stdin/stdout/stderr -> debug console (matches the original HAL). */
    if (fd >= 0 && fd < RXDK_FD_BASE) {
        const char *p = (const char *)buf;
        char line[512];
        size_t n = count;
        if (n >= sizeof(line))
            n = sizeof(line) - 1;
        for (size_t i = 0; i < n; ++i)
            line[i] = p[i];
        line[n] = '\0';
        DbgPrint("%s", line);
        return (ssize_t)count;
    }

    e = get_fd(fd);
    if (!e) { errno = EBADF; return -1; }

    if (e->append) {
        long long sz = file_size(e->handle);
        if (sz >= 0) e->offset = sz;
    }
    off.QuadPart = e->offset;
    st = NtWriteFile(e->handle, NULL, NULL, NULL, &iosb,
                     (PVOID)(size_t)buf, (ULONG)count, &off);
    if (!NT_SUCCESS(st)) { errno = EIO; return -1; }

    e->offset += (long long)iosb.Information;
    return (ssize_t)iosb.Information;
}

off_t lseek(int fd, off_t offset, int whence)
{
    rxdk_fd *e = get_fd(fd);
    long long base;

    if (!e) { errno = EBADF; return -1; }

    switch (whence) {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = e->offset; break;
    case SEEK_END: base = file_size(e->handle); if (base < 0) base = 0; break;
    default: errno = EINVAL; return -1;
    }

    long long pos = base + (long long)offset;
    if (pos < 0) { errno = EINVAL; return -1; }
    e->offset = pos;
    return (off_t)pos;
}

int close(int fd)
{
    rxdk_fd *e = get_fd(fd);
    if (fd >= 0 && fd < RXDK_FD_BASE)
        return 0;
    if (!e) { errno = EBADF; return -1; }
    NtClose(e->handle);
    e->used = 0;
    e->handle = NULL;
    return 0;
}

static void fill_stat(struct stat *st, HANDLE h)
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS s;

    for (size_t i = 0; i < sizeof(*st); ++i)
        ((char *)st)[i] = 0;

    s = NtQueryInformationFile(h, &iosb, &info, sizeof(info),
                               FileStandardInformation);
    if (NT_SUCCESS(s)) {
        st->st_size = (off_t)info.EndOfFile.QuadPart;
        st->st_mode = info.Directory ? S_IFDIR : S_IFREG;
    } else {
        st->st_mode = S_IFREG;
    }
}

int fstat(int fd, struct stat *st)
{
    rxdk_fd *e = get_fd(fd);
    if (!e || !st) { errno = EBADF; return -1; }
    fill_stat(st, e->handle);
    return 0;
}

int stat(const char *path, struct stat *st)
{
    HANDLE h;
    NTSTATUS s;

    if (!st) { errno = EINVAL; return -1; }
    /* No NON_DIRECTORY flag so directories resolve too. */
    s = nt_open(path, NT_GENERIC_READ, NT_FILE_OPEN, 0, &h);
    if (!NT_SUCCESS(s)) { errno = ENOENT; return -1; }
    fill_stat(st, h);
    NtClose(h);
    return 0;
}

static int nt_delete(const char *path, ULONG options)
{
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    FILE_DISPOSITION_INFORMATION dispose;
    NTSTATUS s;

    s = nt_open(path, NT_DELETE, NT_FILE_OPEN, options, &h);
    if (!NT_SUCCESS(s)) { errno = ENOENT; return -1; }

    dispose.DeleteFile = TRUE;
    s = NtSetInformationFile(h, &iosb, &dispose, sizeof(dispose),
                             FileDispositionInformation);
    NtClose(h);
    if (!NT_SUCCESS(s)) { errno = EIO; return -1; }
    return 0;
}

int unlink(const char *path)
{
    return nt_delete(path, NT_FILE_NON_DIRECTORY_FILE);
}

int rmdir(const char *path)
{
    return nt_delete(path, NT_FILE_DIRECTORY_FILE);
}

int mkdir(const char *path, mode_t mode)
{
    HANDLE h;
    NTSTATUS s;

    (void)mode;
    s = nt_open(path, NT_GENERIC_READ, NT_FILE_CREATE, NT_FILE_DIRECTORY_FILE, &h);
    if (!NT_SUCCESS(s)) { errno = EEXIST; return -1; }
    NtClose(h);
    return 0;
}
