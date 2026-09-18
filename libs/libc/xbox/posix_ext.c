/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * POSIX surface the console's libc was missing but portable code links against:
 * directory scanning (scandir/alphasort), fs flush (sync), fpathconf limits, and
 * the process/identity/vm/terminal/permission stubs the OG Xbox has no concept of
 * (one title, no users, no mmap, no tty, permission-less FATX). Each takes the
 * most compatible action -- a fixed value where sensible, otherwise -1/errno.
 * Ported from RXDK-360 runtime/xbox/{dirio,fileio,posix_stubs,signals}.c; the fd/
 * file layer (dup/pread/creat/...) and the C11 sleep live elsewhere (fileio.c /
 * threads.c), and are not duplicated here.
 */
#define _GNU_SOURCE 1
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

/* ---- scandir / alphasort: opendir + readdir into a malloc'd, optionally
   filtered and sorted array of dirent copies. ------------------------------ */
int scandir(const char *dir, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
    DIR *d = opendir(dir);
    struct dirent *ent, **list = NULL, **nl;
    size_t n = 0, cap = 0;
    if (!d)
        return -1;
    while ((ent = readdir(d)) != NULL) {
        struct dirent *copy;
        if (filter && !filter(ent))
            continue;
        if (n == cap) {
            size_t ncap = cap ? cap * 2 : 16;
            nl = (struct dirent **)realloc(list, ncap * sizeof(*list));
            if (!nl)
                goto enomem;
            list = nl;
            cap = ncap;
        }
        copy = (struct dirent *)malloc(sizeof(struct dirent));
        if (!copy)
            goto enomem;
        *copy = *ent;
        list[n++] = copy;
    }
    closedir(d);
    if (compar && n > 1)
        qsort(list, n, sizeof(*list),
              (int (*)(const void *, const void *))compar);
    *namelist = list;
    return (int)n;

enomem:
    while (n)
        free(list[--n]);
    free(list);
    closedir(d);
    errno = ENOMEM;
    return -1;
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

/* ---- sync: writes are already flushed to the kernel synchronously. -------- */
void sync(void) { }

/* ---- fpathconf: fixed limits for the console's FATX/GDFX volumes. --------- */
long fpathconf(int fd, int name)
{
    (void)fd;
    switch (name) {
    case _PC_LINK_MAX:         return 1;        /* no hard links */
    case _PC_NAME_MAX:         return 255;
    case _PC_PATH_MAX:         return PATH_MAX;
    case _PC_PIPE_BUF:         return 4096;
    case _PC_NO_TRUNC:         return 1;
    case _PC_CHOWN_RESTRICTED: return 1;
    default:                   errno = EINVAL; return -1;
    }
}

/* ---- identity: one title, no users. getpid/_exit live in hal.c;
   fork/waitpid in stubs.c. -------------------------------------------------- */
pid_t getppid(void) { return 0; }
uid_t getuid(void)  { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void)  { return 0; }
gid_t getegid(void) { return 0; }

/* ---- no process model: pause has nothing to wake it. --------------------- */
int pause(void) { errno = ENOSYS; return -1; }

/* ---- virtual memory: no mapping API. mprotect succeeds (title memory is
   already accessible to the guest); mmap/munmap report unsupported. sbrk lives
   in hal.c. ----------------------------------------------------------------- */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    (void)addr; (void)len; (void)prot; (void)flags; (void)fd; (void)off;
    errno = ENOSYS;
    return MAP_FAILED;
}
int munmap(void *addr, size_t len)             { (void)addr; (void)len; errno = ENOSYS; return -1; }
int mprotect(void *addr, size_t len, int prot) { (void)addr; (void)len; (void)prot; return 0; }

/* ---- terminals: the console debug channel is not a POSIX tty. ------------- */
int tcgetattr(int fd, struct termios *t)                { (void)fd; (void)t; errno = ENOTTY; return -1; }
int tcsetattr(int fd, int act, const struct termios *t) { (void)fd; (void)act; (void)t; errno = ENOTTY; return -1; }

/* ---- permissions: FATX/GDFX have none, so changes "succeed" as no-ops. ---- */
int    chmod(const char *path, mode_t mode)            { (void)path; (void)mode; return 0; }
int    chown(const char *path, uid_t owner, gid_t grp) { (void)path; (void)owner; (void)grp; return 0; }
mode_t umask(mode_t mask)                              { (void)mask; return 0; }

/* ---- interval timers / alarm: no async signal delivery, so a fresh query
   reports a disarmed timer and arming is a no-op that reports "none pending".
   The firing implementation belongs with SIGALRM delivery (t_signal). ------- */
unsigned int alarm(unsigned int seconds) { (void)seconds; return 0; }

int getitimer(int which, struct itimerval *value)
{
    (void)which;
    if (!value) { errno = EINVAL; return -1; }
    memset(value, 0, sizeof *value);
    return 0;
}

int setitimer(int which, const struct itimerval *value, struct itimerval *old)
{
    (void)which; (void)value;
    if (old)
        memset(old, 0, sizeof *old);
    return 0;
}

/* ---- file-copy helpers: sendfile / copy_file_range are a bounded read+write
   loop over the fd table (no zero-copy path on the console). libc++'s
   <filesystem>::copy_file links against sendfile, so it is needed even before
   t_posix5. posix_fadvise is an accepted no-op; posix_fallocate grows the file;
   renameat only supports AT_FDCWD (FATX has no dir-relative rename). ---------- */
extern ssize_t pread(int, void *, size_t, off_t);
extern ssize_t pwrite(int, const void *, size_t, off_t);
extern int ftruncate(int, off_t);
extern int rename(const char *, const char *);

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
    char buf[4096];
    size_t done = 0;
    off_t pos = offset ? *offset : 0;
    while (done < count) {
        size_t chunk = count - done;
        ssize_t r, w;
        if (chunk > sizeof buf) chunk = sizeof buf;
        r = offset ? pread(in_fd, buf, chunk, pos) : read(in_fd, buf, chunk);
        if (r < 0) return -1;
        if (r == 0) break;
        w = write(out_fd, buf, (size_t)r);
        if (w < 0) return -1;
        done += (size_t)w;
        pos += w;
        if (w < r) break;
    }
    if (offset) *offset = pos;
    return (ssize_t)done;
}

ssize_t copy_file_range(int in_fd, off_t *in_off, int out_fd, off_t *out_off,
                        size_t len, unsigned int flags)
{
    char buf[4096];
    size_t done = 0;
    off_t ip = in_off ? *in_off : 0, op = out_off ? *out_off : 0;
    (void)flags;
    while (done < len) {
        size_t chunk = len - done;
        ssize_t r, w;
        if (chunk > sizeof buf) chunk = sizeof buf;
        r = in_off ? pread(in_fd, buf, chunk, ip) : read(in_fd, buf, chunk);
        if (r < 0) return -1;
        if (r == 0) break;
        w = out_off ? pwrite(out_fd, buf, (size_t)r, op) : write(out_fd, buf, (size_t)r);
        if (w < 0) return -1;
        done += (size_t)w;
        ip += w;
        op += w;
        if (w < r) break;
    }
    if (in_off)  *in_off = ip;
    if (out_off) *out_off = op;
    return (ssize_t)done;
}

int posix_fadvise(int fd, off_t offset, off_t len, int advice)
{
    (void)fd; (void)offset; (void)len; (void)advice;
    return 0;
}

int posix_fallocate(int fd, off_t offset, off_t len)
{
    struct stat st;
    off_t need;
    if (offset < 0 || len <= 0) return EINVAL;
    if (fstat(fd, &st) != 0) return errno;
    need = offset + len;
    if (st.st_size < need && ftruncate(fd, need) != 0) return errno;
    return 0;
}

int renameat(int ofd, const char *oldp, int nfd, const char *newp)
{
    if (ofd != AT_FDCWD || nfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return rename(oldp, newp);
}

/* ---- clock_nanosleep: relative (flags 0) forwards to nanosleep; an absolute
   (TIMER_ABSTIME) deadline is turned into the remaining interval off the same
   clock. Returns 0 or a positive errno directly (does NOT set errno). -------- */
extern int nanosleep(const struct timespec *, struct timespec *);

int clock_nanosleep(clockid_t clk, int flags, const struct timespec *rqtp,
                    struct timespec *rmtp)
{
    if (!rqtp || rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000L)
        return EINVAL;
    if (flags & TIMER_ABSTIME) {
        struct timespec now, d;
        clock_gettime(clk, &now);
        d.tv_sec = rqtp->tv_sec - now.tv_sec;
        d.tv_nsec = rqtp->tv_nsec - now.tv_nsec;
        if (d.tv_nsec < 0) { d.tv_nsec += 1000000000L; d.tv_sec--; }
        if (d.tv_sec < 0 || (d.tv_sec == 0 && d.tv_nsec <= 0))
            return 0;                     /* deadline already passed */
        return nanosleep(&d, NULL) == 0 ? 0 : errno;
    }
    return nanosleep(rqtp, rmtp) == 0 ? 0 : errno;
}

/* ---- sched_yield: hand the core to another ready thread (C11 thrd_yield). --- */
extern void thrd_yield(void);
int sched_yield(void) { thrd_yield(); return 0; }

/* ---- getrandom: the console has no HW entropy; loop getentropy (256B cap). -- */
extern int getentropy(void *buf, size_t len);
ssize_t getrandom(void *buf, size_t len, unsigned int flags)
{
    unsigned char *p = (unsigned char *)buf;
    size_t left = len;
    (void)flags;
    if (!buf && len) { errno = EFAULT; return -1; }
    while (left) {
        size_t chunk = left < 256 ? left : 256;
        getentropy(p, chunk);
        p += chunk;
        left -= chunk;
    }
    return (ssize_t)len;
}

/* ---- confstr: one fixed environment; report the launch drive for _CS_PATH,
   empty strings otherwise, returning the full length (+ NUL). --------------- */
size_t confstr(int name, char *buf, size_t len)
{
    const char *val = (name == _CS_PATH) ? "D:\\" : "";
    size_t n = strlen(val);
    if (buf && len) {
        size_t c = (n < len - 1) ? n : len - 1;
        memcpy(buf, val, c);
        buf[c] = '\0';
    }
    return n + 1;
}

/* ---- mkdtemp / mkostemp: fill the six trailing 'X's with random [a-z0-9] and
   retry on collision. mkdtemp makes a directory, mkostemp an O_EXCL file. ---- */
extern long random(void);

static int rxdk_fill_template(char *tmpl)
{
    static const char cset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    size_t len = strlen(tmpl);
    char *x;
    int i;
    if (len < 6) { errno = EINVAL; return -1; }
    x = tmpl + len - 6;
    for (i = 0; i < 6; i++)
        if (x[i] != 'X') { errno = EINVAL; return -1; }
    for (i = 0; i < 6; i++)
        x[i] = cset[(unsigned long)random() % 36];
    return 0;
}

char *mkdtemp(char *tmpl)
{
    int attempt;
    if (rxdk_fill_template(tmpl) != 0)
        return NULL;
    for (attempt = 0; attempt < 128; attempt++) {
        if (mkdir(tmpl, 0700) == 0)
            return tmpl;
        if (errno != EEXIST)
            return NULL;
        if (rxdk_fill_template(tmpl) != 0)
            return NULL;
    }
    errno = EEXIST;
    return NULL;
}

int mkostemp(char *tmpl, int flags)
{
    int attempt, fd;
    if (rxdk_fill_template(tmpl) != 0)
        return -1;
    for (attempt = 0; attempt < 128; attempt++) {
        fd = open(tmpl, O_CREAT | O_EXCL | O_RDWR | flags, 0600);
        if (fd >= 0)
            return fd;
        if (errno != EEXIST)
            return -1;
        if (rxdk_fill_template(tmpl) != 0)
            return -1;
    }
    errno = EEXIST;
    return -1;
}
