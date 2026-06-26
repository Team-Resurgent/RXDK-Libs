#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef unsigned long sigset_t;

int getentropy(void *buf, size_t len)
{
    (void)buf;
    (void)len;
    errno = ENOSYS;
    return -1;
}

int raise(int sig)
{
    (void)sig;
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    (void)how;
    (void)set;
    (void)oldset;
    return 0;
}

int stat(const char *path, struct stat *st)
{
    (void)path;
    (void)st;
    errno = ENOENT;
    return -1;
}

int fstat(int fd, struct stat *st)
{
    (void)fd;
    (void)st;
    errno = ENOSYS;
    return -1;
}

typedef struct {
    int dummy;
} regex_t;

typedef struct {
    int dummy;
} regmatch_t;

int regcomp(regex_t *preg, const char *pattern, int cflags)
{
    (void)preg;
    (void)pattern;
    (void)cflags;
    return 0;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
    (void)preg;
    (void)string;
    (void)nmatch;
    (void)pmatch;
    (void)eflags;
    return 1;
}

void regfree(regex_t *preg)
{
    (void)preg;
}

int open(const char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    errno = ENOENT;
    return -1;
}

int pipe(int fildes[2])
{
    (void)fildes;
    errno = ENOSYS;
    return -1;
}

int fork(void)
{
    errno = ENOSYS;
    return -1;
}

int dup2(int oldfd, int newfd)
{
    (void)oldfd;
    (void)newfd;
    errno = ENOSYS;
    return -1;
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    (void)path;
    (void)argv;
    (void)envp;
    errno = ENOSYS;
    return -1;
}

int waitpid(int pid, int *status, int options)
{
    (void)pid;
    (void)status;
    (void)options;
    errno = ENOSYS;
    return -1;
}

ssize_t read(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

int close(int fd)
{
    (void)fd;
    errno = ENOSYS;
    return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ENOSYS;
    return -1;
}

int unlink(const char *path)
{
    (void)path;
    errno = ENOENT;
    return -1;
}

long times(void *buf)
{
    (void)buf;
    return -1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    (void)clk_id;
    if (tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 0;
    }
    return 0;
}

int clock_getres(clockid_t clk_id, struct timespec *tp)
{
    (void)clk_id;
    if (tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 1000000000;
    }
    return 0;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (tv) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }
    return 0;
}
