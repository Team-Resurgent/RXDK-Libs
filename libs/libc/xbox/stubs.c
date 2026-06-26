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

/* stat/fstat live in fileio.c (kernel-backed). */
/* regcomp/regexec/regerror/regfree are picolibc's POSIX regex (libc/posix/). */
/* open() lives in fileio.c (kernel-backed). */

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

/* read/close/lseek/unlink live in fileio.c (kernel-backed). */

long times(void *buf)
{
    (void)buf;
    return -1;
}

/* clock_gettime/clock_getres/gettimeofday/clock live in timeio.c (kernel-backed). */
