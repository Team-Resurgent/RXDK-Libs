#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include "xbox/kernel.h"

extern char *__enviro;

char *__enviro = (char *)0;

static char *heap_end;
static char heap_buf[256 * 1024];

/* write() lives in fileio.c (it now routes fd>=3 to real files). */

void *sbrk(ptrdiff_t incr)
{
    char *base = heap_end ? heap_end : heap_buf;
    char *next = base + incr;

    if (next < heap_buf || next > heap_buf + sizeof(heap_buf)) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end = next;
    return base;
}

void _exit(int status) /* x86-windows-gnu exports as __exit; crt0.S calls __exit */
{
    (void)status;
    DbgPrint("RXDK-LibsZig: _exit\n");
    for (;;)
        ;
}

int getpid(void)
{
    return 1;
}

int kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

int isatty(int fd)
{
    (void)fd;
    return 1;
}
