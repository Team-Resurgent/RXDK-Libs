#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include "xbox/kernel.h"

/* NT virtual-memory flags, not exposed by the slim kernel headers (ntddk.h). */
#ifndef MEM_COMMIT
#define MEM_COMMIT      0x00001000UL
#endif
#ifndef MEM_RESERVE
#define MEM_RESERVE     0x00002000UL
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE  0x04UL
#endif

extern char *__enviro;

char *__enviro = (char *)0;

/*
 * Program break / heap, backed by the kernel virtual memory manager.
 *
 * We reserve a large virtual window once (MEM_RESERVE), then commit pages on
 * demand as the break advances (MEM_COMMIT). Per the Xbox MM source, a reserve
 * costs only a single VAD and an accounting counter -- it does NOT consume
 * physical RAM -- so the window is just a ceiling; real RAM tracks actual
 * allocation. There is no pagefile, so committing past available RAM fails
 * cleanly: sbrk returns -1 and malloc reports ENOMEM. Kernel-only: this layer
 * never calls into libxapi, keeping libc -> kernel layering acyclic.
 *
 * The reservation is free, so the ceiling can be generous; tune as needed.
 */
#define RXDK_HEAP_RESERVE_BYTES (32u * 1024u * 1024u)

static char *heap_base;   /* start of the reserved window (kernel-chosen) */
static char *heap_end;    /* current program break */
static char *heap_commit; /* end of committed pages (always >= heap_end) */
static char *heap_limit;  /* end of the reserved window */

/* write() lives in fileio.c (it now routes fd>=3 to real files). */

static int heap_reserve(void)
{
    PVOID base = (PVOID)0;
    SIZE_T size = RXDK_HEAP_RESERVE_BYTES;
    NTSTATUS status =
        NtAllocateVirtualMemory(&base, 0, &size, MEM_RESERVE, PAGE_READWRITE);

    if (!NT_SUCCESS(status) || base == (PVOID)0)
        return -1;

    heap_base = (char *)base;
    heap_end = heap_base;
    heap_commit = heap_base;
    heap_limit = heap_base + size;
    return 0;
}

void *sbrk(ptrdiff_t incr)
{
    char *prev;
    char *next;

    if (heap_base == (char *)0 && heap_reserve() != 0) {
        errno = ENOMEM;
        return (void *)-1;
    }

    prev = heap_end;
    next = heap_end + incr;

    if (next < heap_base || next > heap_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }

    /* Commit the pages the new break reaches into (idempotent past the mark). */
    if (next > heap_commit) {
        PVOID cbase = (PVOID)heap_commit;
        SIZE_T csize = (SIZE_T)(next - heap_commit);
        NTSTATUS status =
            NtAllocateVirtualMemory(&cbase, 0, &csize, MEM_COMMIT, PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            errno = ENOMEM;
            return (void *)-1;
        }
        /* csize is rounded up to a page boundary; advance the commit mark. */
        heap_commit = (char *)cbase + csize;
    }

    heap_end = next;
    return prev;
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
