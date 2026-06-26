/*
 * picolibc retargetable locking on the Xbox kernel.
 *
 * With __SINGLE_THREAD off, picolibc serializes its internals (malloc, stdio
 * buffers, atexit, env, ...) through these __retarget_lock_* hooks plus a set
 * of predefined static lock objects. Each lock is an RTL_CRITICAL_SECTION
 * (recursive), lazily initialized on first acquire — safe because every
 * predefined lock is first touched on the main thread before any worker spawns.
 *
 * Kernel-only; no libxapi.
 */

#include <stdlib.h>
#include <sys/lock.h>

#include "xbox/kernel.h"

struct __lock {
    int inited;
    RTL_CRITICAL_SECTION cs;
};

/* Predefined recursive mutexes picolibc references by name. */
struct __lock __lock___libc_recursive_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___arc4random_mutex;

static void ensure(struct __lock *l)
{
    if (!l->inited) {
        RtlInitializeCriticalSection(&l->cs);
        l->inited = 1;
    }
}

void __retarget_lock_init(_LOCK_T *lock)
{
    struct __lock *l = (struct __lock *)malloc(sizeof(*l));
    if (l) {
        l->inited = 0;
        ensure(l);
    }
    *lock = l;
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
    __retarget_lock_init(lock);
}

void __retarget_lock_close(_LOCK_T lock)
{
    free(lock); /* no RtlDeleteCriticalSection on Xbox */
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    free(lock);
}

void __retarget_lock_acquire(_LOCK_T lock)
{
    ensure(lock);
    RtlEnterCriticalSection(&lock->cs);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    ensure(lock);
    RtlEnterCriticalSection(&lock->cs);
}

void __retarget_lock_release(_LOCK_T lock)
{
    RtlLeaveCriticalSection(&lock->cs);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
    RtlLeaveCriticalSection(&lock->cs);
}
