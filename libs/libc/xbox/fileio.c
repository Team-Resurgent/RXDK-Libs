/*
 * POSIX file I/O backend for picolibc, implemented directly on the Xbox kernel
 * (Nt* / Io* / Rtl* exports). This is the storage layer picolibc stdio calls
 * down into (fopen/fread/fwrite/fseek/...). It deliberately uses ONLY the
 * kernel + picolibc headers — never libxapi — so the libc -> kernel layering
 * stays acyclic (libxapi depends on libc, not the reverse).
 *
 * fds are shared, refcounted "open file descriptions" so dup2 can alias them.
 * A description is either a kernel file handle or an in-process pipe. read(0)
 * and write(1/2) route through the registered libc hooks when present.
 */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "xbox/kernel.h"
#include "xbox/libc_hooks.h"

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

#define RXDK_FD_BASE   3
#define RXDK_FD_MAX    32
#define RXDK_PIPE_SIZE 4096

#define RXDK_KIND_FILE 0
#define RXDK_KIND_PIPE 1

/* In-process pipe: a ring buffer with reader/writer end counts. */
struct rxdk_pipe {
    unsigned char buf[RXDK_PIPE_SIZE];
    unsigned int head, tail, count;
    int readers, writers;
    RTL_CRITICAL_SECTION lock;
    KEVENT data_ev;  /* signaled when data becomes available */
    KEVENT space_ev; /* signaled when space becomes available */
};

/* Open file description (shared by dup'd fds). */
typedef struct rxdk_ofd {
    int refcount;
    int kind;
    /* file */
    HANDLE handle;
    long long offset;
    int append;
    /* pipe */
    struct rxdk_pipe *pipe;
    int write_end;
} rxdk_ofd;

static rxdk_ofd *fd_table[RXDK_FD_MAX]; /* NULL = free */

static int alloc_slot(rxdk_ofd *o)
{
    for (int i = RXDK_FD_BASE; i < RXDK_FD_MAX; ++i) {
        if (!fd_table[i]) {
            fd_table[i] = o;
            return i;
        }
    }
    return -1;
}

static rxdk_ofd *get_fd(int fd)
{
    if (fd < RXDK_FD_BASE || fd >= RXDK_FD_MAX)
        return NULL;
    return fd_table[fd];
}

static rxdk_ofd *new_ofd(int kind)
{
    rxdk_ofd *o = (rxdk_ofd *)malloc(sizeof(*o));
    if (!o)
        return NULL;
    o->refcount = 1;
    o->kind = kind;
    o->handle = NULL;
    o->offset = 0;
    o->append = 0;
    o->pipe = NULL;
    o->write_end = 0;
    return o;
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

/* ---- pipe ring buffer ------------------------------------------------------ */

static ssize_t pipe_read(struct rxdk_pipe *p, void *buf, size_t count)
{
    unsigned char *out = (unsigned char *)buf;
    size_t n = 0;

    RtlEnterCriticalSection(&p->lock);
    while (p->count == 0) {
        if (p->writers == 0) { /* all write ends closed -> EOF */
            RtlLeaveCriticalSection(&p->lock);
            return 0;
        }
        RtlLeaveCriticalSection(&p->lock);
        KeWaitForSingleObject(&p->data_ev, 0, 0, FALSE, NULL);
        RtlEnterCriticalSection(&p->lock);
    }
    while (n < count && p->count > 0) {
        out[n++] = p->buf[p->head];
        p->head = (p->head + 1) % RXDK_PIPE_SIZE;
        p->count--;
    }
    RtlLeaveCriticalSection(&p->lock);
    KeSetEvent(&p->space_ev, 0, FALSE);
    return (ssize_t)n;
}

static ssize_t pipe_write(struct rxdk_pipe *p, const void *buf, size_t count)
{
    const unsigned char *in = (const unsigned char *)buf;
    size_t n = 0;

    RtlEnterCriticalSection(&p->lock);
    while (n < count) {
        while (p->count == RXDK_PIPE_SIZE) {
            if (p->readers == 0) { /* no readers left -> broken pipe */
                RtlLeaveCriticalSection(&p->lock);
                errno = EPIPE;
                return n > 0 ? (ssize_t)n : -1;
            }
            RtlLeaveCriticalSection(&p->lock);
            KeWaitForSingleObject(&p->space_ev, 0, 0, FALSE, NULL);
            RtlEnterCriticalSection(&p->lock);
        }
        while (n < count && p->count < RXDK_PIPE_SIZE) {
            p->buf[p->tail] = in[n++];
            p->tail = (p->tail + 1) % RXDK_PIPE_SIZE;
            p->count++;
        }
        KeSetEvent(&p->data_ev, 0, FALSE);
    }
    RtlLeaveCriticalSection(&p->lock);
    return (ssize_t)n;
}

/* Drop one end of a pipe (when its description is freed). */
static void pipe_detach(rxdk_ofd *o)
{
    struct rxdk_pipe *p = o->pipe;
    int dead;

    RtlEnterCriticalSection(&p->lock);
    if (o->write_end)
        p->writers--;
    else
        p->readers--;
    /* wake anyone blocked so they observe the closed end */
    KeSetEvent(&p->data_ev, 0, FALSE);
    KeSetEvent(&p->space_ev, 0, FALSE);
    dead = (p->readers == 0 && p->writers == 0);
    RtlLeaveCriticalSection(&p->lock);
    if (dead)
        free(p);
}

/* ---- file open ------------------------------------------------------------- */

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
    rxdk_ofd *o;
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

    o = new_ofd(RXDK_KIND_FILE);
    if (!o) {
        NtClose(h);
        errno = ENOMEM;
        return -1;
    }
    o->handle = h;
    if (flags & O_APPEND) {
        long long sz = file_size(h);
        o->append = 1;
        o->offset = (sz > 0) ? sz : 0;
    }

    fd = alloc_slot(o);
    if (fd < 0) {
        NtClose(h);
        free(o);
        errno = EMFILE;
        return -1;
    }
    return fd;
}

/* ---- read / write / lseek / close ------------------------------------------ */

ssize_t read(int fd, void *buf, size_t count)
{
    rxdk_ofd *o;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    NTSTATUS st;

    if (fd == 0) { /* stdin: app-registered handler, else EOF */
        if (__rxdk_stdin_hook)
            return __rxdk_stdin_hook(buf, count);
        return 0;
    }

    o = get_fd(fd);
    if (!o) { errno = EBADF; return -1; }
    if (count == 0) return 0;

    if (o->kind == RXDK_KIND_PIPE)
        return pipe_read(o->pipe, buf, count);

    off.QuadPart = o->offset;
    st = NtReadFile(o->handle, NULL, NULL, NULL, &iosb, buf, (ULONG)count, &off);
    if (st == STATUS_END_OF_FILE)
        return 0;
    if (!NT_SUCCESS(st)) { errno = EIO; return -1; }

    o->offset += (long long)iosb.Information;
    return (ssize_t)iosb.Information;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    rxdk_ofd *o;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER off;
    NTSTATUS st;

    if (!buf || count == 0)
        return 0;

    /* stdin/stdout/stderr: app output handler, else the debug console. */
    if (fd >= 0 && fd < RXDK_FD_BASE) {
        const char *p = (const char *)buf;
        char line[512];
        size_t n = count;

        if (__rxdk_output_hook)
            return __rxdk_output_hook(fd, buf, count);

        if (n >= sizeof(line))
            n = sizeof(line) - 1;
        for (size_t i = 0; i < n; ++i)
            line[i] = p[i];
        line[n] = '\0';
        DbgPrint("%s", line);
        return (ssize_t)count;
    }

    o = get_fd(fd);
    if (!o) { errno = EBADF; return -1; }

    if (o->kind == RXDK_KIND_PIPE)
        return pipe_write(o->pipe, buf, count);

    if (o->append) {
        long long sz = file_size(o->handle);
        if (sz >= 0) o->offset = sz;
    }
    off.QuadPart = o->offset;
    st = NtWriteFile(o->handle, NULL, NULL, NULL, &iosb,
                     (PVOID)(size_t)buf, (ULONG)count, &off);
    if (!NT_SUCCESS(st)) { errno = EIO; return -1; }

    o->offset += (long long)iosb.Information;
    return (ssize_t)iosb.Information;
}

off_t lseek(int fd, off_t offset, int whence)
{
    rxdk_ofd *o = get_fd(fd);
    long long base;

    if (!o) { errno = EBADF; return -1; }
    if (o->kind == RXDK_KIND_PIPE) { errno = ESPIPE; return -1; }

    switch (whence) {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = o->offset; break;
    case SEEK_END: base = file_size(o->handle); if (base < 0) base = 0; break;
    default: errno = EINVAL; return -1;
    }

    long long pos = base + (long long)offset;
    if (pos < 0) { errno = EINVAL; return -1; }
    o->offset = pos;
    return (off_t)pos;
}

int close(int fd)
{
    rxdk_ofd *o;

    if (fd >= 0 && fd < RXDK_FD_BASE)
        return 0;
    o = get_fd(fd);
    if (!o) { errno = EBADF; return -1; }

    fd_table[fd] = NULL;
    if (--o->refcount == 0) {
        if (o->kind == RXDK_KIND_PIPE)
            pipe_detach(o);
        else if (o->handle)
            NtClose(o->handle);
        free(o);
    }
    return 0;
}

/* ---- pipe() / dup2() ------------------------------------------------------- */

int pipe(int fildes[2])
{
    struct rxdk_pipe *p;
    rxdk_ofd *rd;
    rxdk_ofd *wr;
    int r, w;

    if (!fildes) { errno = EFAULT; return -1; }

    p = (struct rxdk_pipe *)malloc(sizeof(*p));
    rd = new_ofd(RXDK_KIND_PIPE);
    wr = new_ofd(RXDK_KIND_PIPE);
    if (!p || !rd || !wr) {
        free(p);
        free(rd);
        free(wr);
        errno = ENFILE;
        return -1;
    }

    p->head = p->tail = p->count = 0;
    p->readers = 1;
    p->writers = 1;
    RtlInitializeCriticalSection(&p->lock);
    KeInitializeEvent(&p->data_ev, SynchronizationEvent, FALSE);
    KeInitializeEvent(&p->space_ev, SynchronizationEvent, FALSE);

    rd->pipe = p;
    rd->write_end = 0;
    wr->pipe = p;
    wr->write_end = 1;

    r = alloc_slot(rd);
    w = alloc_slot(wr);
    if (r < 0 || w < 0) {
        if (r >= 0) fd_table[r] = NULL;
        if (w >= 0) fd_table[w] = NULL;
        free(rd);
        free(wr);
        free(p);
        errno = EMFILE;
        return -1;
    }
    fildes[0] = r;
    fildes[1] = w;
    return 0;
}

int dup2(int oldfd, int newfd)
{
    rxdk_ofd *o;

    /* console fds all route to the same place; aliasing among them is a no-op */
    if (oldfd >= 0 && oldfd < RXDK_FD_BASE &&
        newfd >= 0 && newfd < RXDK_FD_BASE)
        return newfd;

    o = get_fd(oldfd);
    if (!o) { errno = EBADF; return -1; }
    if (newfd < RXDK_FD_BASE || newfd >= RXDK_FD_MAX) { errno = EBADF; return -1; }
    if (oldfd == newfd) return newfd;

    if (fd_table[newfd])
        close(newfd);

    o->refcount++;
    fd_table[newfd] = o;
    return newfd;
}

/* ---- stat family ----------------------------------------------------------- */

static void set_stat(struct stat *st, long long size, unsigned long attrs)
{
    for (size_t i = 0; i < sizeof(*st); ++i)
        ((char *)st)[i] = 0;
    st->st_size = (off_t)size;
    st->st_mode = (attrs & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG;
}

int fstat(int fd, struct stat *st)
{
    rxdk_ofd *o = get_fd(fd);
    FILE_STANDARD_INFORMATION sinfo;
    FILE_BASIC_INFORMATION binfo;
    IO_STATUS_BLOCK iosb;
    long long size = 0;
    unsigned long attrs = 0;

    if (!o || !st) { errno = EBADF; return -1; }

    if (o->kind == RXDK_KIND_PIPE) {
        set_stat(st, 0, 0);
        st->st_mode = S_IFIFO;
        return 0;
    }

    if (NT_SUCCESS(NtQueryInformationFile(o->handle, &iosb, &sinfo,
                                          sizeof(sinfo), FileStandardInformation)))
        size = (long long)sinfo.EndOfFile.QuadPart;
    if (NT_SUCCESS(NtQueryInformationFile(o->handle, &iosb, &binfo,
                                          sizeof(binfo), FileBasicInformation)))
        attrs = binfo.FileAttributes;

    set_stat(st, size, attrs);
    return 0;
}

int stat(const char *path, struct stat *st)
{
    OBJECT_STRING name;
    OBJECT_ATTRIBUTES obja;
    FILE_NETWORK_OPEN_INFORMATION info;
    NTSTATUS s;

    if (!st) { errno = EINVAL; return -1; }

    RtlInitAnsiString(&name, path);
    InitializeObjectAttributes(&obja, &name, OBJ_CASE_INSENSITIVE,
                               ObDosDevicesDirectory(), NULL);
    s = NtQueryFullAttributesFile(&obja, &info);
    if (!NT_SUCCESS(s)) { errno = ENOENT; return -1; }

    set_stat(st, (long long)info.EndOfFile.QuadPart, info.FileAttributes);
    return 0;
}

/* ---- unlink / mkdir / rmdir ------------------------------------------------ */

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
