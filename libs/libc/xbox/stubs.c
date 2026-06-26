#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "xbox/kernel.h"

typedef unsigned long sigset_t;

/*
 * getentropy: the Xbox kernel exposes no CSPRNG, so this derives bytes from the
 * performance counter (which advances every read) mixed with system time and
 * the tick count, then run through a splitmix64-style finalizer for diffusion.
 * Non-deterministic and well-distributed, but NOT cryptographically secure.
 */
int getentropy(void *buf, size_t len)
{
    unsigned char *p = (unsigned char *)buf;
    size_t i = 0;

    if (len > 256) { /* getentropy contract: at most 256 bytes */
        errno = EIO;
        return -1;
    }

    while (i < len) {
        LARGE_INTEGER t;
        unsigned long long mix;
        int b;

        KeQuerySystemTime(&t);
        mix = (unsigned long long)KeQueryPerformanceCounter();
        mix ^= (unsigned long long)t.QuadPart;
        mix ^= (unsigned long long)KeTickCount << 17;
        mix ^= (unsigned long long)i * 0x9E3779B97F4A7C15ULL;
        mix ^= mix >> 33;
        mix *= 0xFF51AFD7ED558CCDULL;
        mix ^= mix >> 33;

        for (b = 0; b < 8 && i < len; ++b, ++i)
            p[i] = (unsigned char)(mix >> (b * 8));
    }
    return 0;
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
/* times + clock_gettime/clock_getres/gettimeofday/clock live in timeio.c. */
