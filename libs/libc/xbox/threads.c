/*
 * C11 <threads.h> on the Xbox kernel.
 *
 *   thrd_*  -> PsCreateSystemThreadEx / PsTerminateSystemThread, join via
 *              NtWaitForSingleObject on the thread handle
 *   mtx_*   -> RTL_CRITICAL_SECTION (always recursive)
 *   cnd_*   -> KEVENT (SynchronizationEvent) + waiter count
 *
 * Kernel-only; no libxapi. tss_* awaits per-thread TLS (Phase 2) and currently
 * reports thrd_error.
 */

#include <stdlib.h>
#include <threads.h>

#include "xbox/kernel.h"

/* The public opaque buffers must hold the real kernel objects. */
_Static_assert(sizeof(RTL_CRITICAL_SECTION) <= sizeof(((mtx_t *)0)->__opaque),
               "mtx_t too small for RTL_CRITICAL_SECTION");

#define RXDK_THREAD_STACK 0x10000 /* 64 KiB per thread */

struct __rxdk_thrd {
    HANDLE handle;
    thrd_start_t func;
    void *arg;
    int result;
    int detached;
};

struct rxdk_cnd {
    KEVENT ev;
    volatile long waiters;
};
_Static_assert(sizeof(struct rxdk_cnd) <= sizeof(((cnd_t *)0)->__opaque),
               "cnd_t too small");

/* ---- current-thread registry (maps KTHREAD -> TCB for thrd_current) -------- */

#define RXDK_MAX_THREADS 64

static struct {
    PKTHREAD kt;
    struct __rxdk_thrd *t;
} g_reg[RXDK_MAX_THREADS];

static RTL_CRITICAL_SECTION g_reg_lock;
static int g_reg_lock_init; /* first touch is single-threaded (main) */

static void reg_lock(void)
{
    if (!g_reg_lock_init) {
        RtlInitializeCriticalSection(&g_reg_lock);
        g_reg_lock_init = 1;
    }
    RtlEnterCriticalSection(&g_reg_lock);
}

static void reg_unlock(void)
{
    RtlLeaveCriticalSection(&g_reg_lock);
}

static void reg_add(struct __rxdk_thrd *t)
{
    PKTHREAD kt = KeGetCurrentThread();
    reg_lock();
    for (int i = 0; i < RXDK_MAX_THREADS; ++i) {
        if (g_reg[i].t == NULL) {
            g_reg[i].kt = kt;
            g_reg[i].t = t;
            break;
        }
    }
    reg_unlock();
}

static void reg_del(struct __rxdk_thrd *t)
{
    reg_lock();
    for (int i = 0; i < RXDK_MAX_THREADS; ++i) {
        if (g_reg[i].t == t) {
            g_reg[i].t = NULL;
            g_reg[i].kt = NULL;
            break;
        }
    }
    reg_unlock();
}

/* ---- threads --------------------------------------------------------------- */

static VOID NTAPI thread_entry(PVOID ctx)
{
    struct __rxdk_thrd *t = (struct __rxdk_thrd *)ctx;
    reg_add(t);
    t->result = t->func(t->arg);
    reg_del(t);
}

static VOID NTAPI thread_trampoline(PKSTART_ROUTINE start, PVOID ctx)
{
    start(ctx);
    PsTerminateSystemThread(0);
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct __rxdk_thrd *t;
    HANDLE h;
    NTSTATUS s;

    if (!thr || !func)
        return thrd_error;

    t = (struct __rxdk_thrd *)malloc(sizeof(*t));
    if (!t)
        return thrd_nomem;
    t->handle = NULL;
    t->func = func;
    t->arg = arg;
    t->result = 0;
    t->detached = 0;

    s = PsCreateSystemThreadEx(&h, 0, RXDK_THREAD_STACK, 0, NULL,
                               thread_entry, t, FALSE, FALSE, thread_trampoline);
    if (!NT_SUCCESS(s)) {
        free(t);
        return thrd_error;
    }
    t->handle = h;
    *thr = t;
    return thrd_success;
}

int thrd_join(thrd_t thr, int *res)
{
    if (!thr)
        return thrd_error;
    NtWaitForSingleObject(thr->handle, FALSE, NULL);
    if (res)
        *res = thr->result;
    NtClose(thr->handle);
    free(thr);
    return thrd_success;
}

int thrd_detach(thrd_t thr)
{
    if (!thr)
        return thrd_error;
    /* The running thread still owns the TCB; closing the handle is enough to
       release the kernel object. The small TCB is reclaimed at process exit. */
    thr->detached = 1;
    NtClose(thr->handle);
    return thrd_success;
}

thrd_t thrd_current(void)
{
    PKTHREAD kt = KeGetCurrentThread();
    thrd_t found = NULL;
    reg_lock();
    for (int i = 0; i < RXDK_MAX_THREADS; ++i) {
        if (g_reg[i].kt == kt) {
            found = g_reg[i].t;
            break;
        }
    }
    reg_unlock();
    return found;
}

int thrd_equal(thrd_t a, thrd_t b)
{
    return a == b;
}

_Noreturn void thrd_exit(int res)
{
    struct __rxdk_thrd *t = thrd_current();
    if (t) {
        t->result = res;
        reg_del(t);
    }
    PsTerminateSystemThread(0);
    for (;;) /* PsTerminateSystemThread does not return */
        ;
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    LARGE_INTEGER interval;
    if (remaining) {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    if (!duration)
        return -1;
    /* relative 100ns ticks are negative */
    interval.QuadPart = -((long long)duration->tv_sec * 10000000LL
                          + (long long)duration->tv_nsec / 100LL);
    KeDelayExecutionThread(0, FALSE, &interval);
    return 0;
}

void thrd_yield(void)
{
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    KeDelayExecutionThread(0, FALSE, &zero);
}

/* ---- mutexes --------------------------------------------------------------- */

int mtx_init(mtx_t *mtx, int type)
{
    (void)type; /* RTL critical sections are always recursive */
    if (!mtx)
        return thrd_error;
    RtlInitializeCriticalSection((PRTL_CRITICAL_SECTION)mtx);
    return thrd_success;
}

int mtx_lock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)mtx);
    return thrd_success;
}

int mtx_trylock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;
    return RtlTryEnterCriticalSection((PRTL_CRITICAL_SECTION)mtx) ? thrd_success
                                                                  : thrd_busy;
}

int mtx_timedlock(mtx_t *restrict mtx, const struct timespec *restrict ts)
{
    (void)ts; /* timeout not yet honored; block */
    return mtx_lock(mtx);
}

int mtx_unlock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)mtx);
    return thrd_success;
}

void mtx_destroy(mtx_t *mtx)
{
    (void)mtx; /* no RtlDeleteCriticalSection on Xbox */
}

/* ---- condition variables --------------------------------------------------- */

int cnd_init(cnd_t *cond)
{
    struct rxdk_cnd *c = (struct rxdk_cnd *)cond;
    if (!c)
        return thrd_error;
    KeInitializeEvent(&c->ev, SynchronizationEvent, FALSE);
    c->waiters = 0;
    return thrd_success;
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
    struct rxdk_cnd *c = (struct rxdk_cnd *)cond;
    if (!c || !mtx)
        return thrd_error;
    c->waiters++; /* caller holds mtx */
    mtx_unlock(mtx);
    KeWaitForSingleObject(&c->ev, 0, 0, FALSE, NULL);
    mtx_lock(mtx);
    c->waiters--;
    return thrd_success;
}

int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx,
                  const struct timespec *restrict ts)
{
    (void)ts; /* timeout not yet honored */
    return cnd_wait(cond, mtx);
}

int cnd_signal(cnd_t *cond)
{
    struct rxdk_cnd *c = (struct rxdk_cnd *)cond;
    if (!c)
        return thrd_error;
    if (c->waiters > 0)
        KeSetEvent(&c->ev, 0, FALSE);
    return thrd_success;
}

int cnd_broadcast(cnd_t *cond)
{
    struct rxdk_cnd *c = (struct rxdk_cnd *)cond;
    long n;
    if (!c)
        return thrd_error;
    for (n = c->waiters; n > 0; --n)
        KeSetEvent(&c->ev, 0, FALSE);
    return thrd_success;
}

void cnd_destroy(cnd_t *cond)
{
    (void)cond;
}

/* ---- call_once ------------------------------------------------------------- */

static RTL_CRITICAL_SECTION g_once_lock;
static int g_once_lock_init;

void call_once(once_flag *flag, void (*func)(void))
{
    if (!g_once_lock_init) {
        RtlInitializeCriticalSection(&g_once_lock);
        g_once_lock_init = 1;
    }
    RtlEnterCriticalSection(&g_once_lock);
    if (flag->__state == 0) {
        func(); /* run under the lock so racing callers block until done */
        flag->__state = 1;
    }
    RtlLeaveCriticalSection(&g_once_lock);
}

/* ---- thread-specific storage ----------------------------------------------- */
/* Per-thread values keyed by (KTHREAD, key) in a flat table — no kernel TLS
   needed. Keys are 1-based; 0 is invalid. Destructors are not yet run at thread
   exit. Guarded by the same registry lock. */

#define RXDK_TSS_KEYS  32
#define RXDK_TSS_SLOTS 256

static struct {
    int used;
    tss_dtor_t dtor;
} g_tss_keys[RXDK_TSS_KEYS];

static struct {
    int inuse;
    PKTHREAD kt;
    tss_t key;
    void *val;
} g_tss_slots[RXDK_TSS_SLOTS];

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    if (!key)
        return thrd_error;
    reg_lock();
    for (unsigned i = 0; i < RXDK_TSS_KEYS; ++i) {
        if (!g_tss_keys[i].used) {
            g_tss_keys[i].used = 1;
            g_tss_keys[i].dtor = dtor;
            *key = i + 1;
            reg_unlock();
            return thrd_success;
        }
    }
    reg_unlock();
    return thrd_error;
}

int tss_set(tss_t key, void *val)
{
    PKTHREAD kt = KeGetCurrentThread();
    int free_slot = -1;

    if (key == 0 || key > RXDK_TSS_KEYS)
        return thrd_error;
    reg_lock();
    for (int i = 0; i < RXDK_TSS_SLOTS; ++i) {
        if (g_tss_slots[i].inuse && g_tss_slots[i].kt == kt &&
            g_tss_slots[i].key == key) {
            g_tss_slots[i].val = val;
            reg_unlock();
            return thrd_success;
        }
        if (!g_tss_slots[i].inuse && free_slot < 0)
            free_slot = i;
    }
    if (free_slot >= 0) {
        g_tss_slots[free_slot].inuse = 1;
        g_tss_slots[free_slot].kt = kt;
        g_tss_slots[free_slot].key = key;
        g_tss_slots[free_slot].val = val;
        reg_unlock();
        return thrd_success;
    }
    reg_unlock();
    return thrd_error;
}

void *tss_get(tss_t key)
{
    PKTHREAD kt = KeGetCurrentThread();
    void *v = NULL;

    if (key == 0 || key > RXDK_TSS_KEYS)
        return NULL;
    reg_lock();
    for (int i = 0; i < RXDK_TSS_SLOTS; ++i) {
        if (g_tss_slots[i].inuse && g_tss_slots[i].kt == kt &&
            g_tss_slots[i].key == key) {
            v = g_tss_slots[i].val;
            break;
        }
    }
    reg_unlock();
    return v;
}

void tss_delete(tss_t key)
{
    if (key == 0 || key > RXDK_TSS_KEYS)
        return;
    reg_lock();
    g_tss_keys[key - 1].used = 0;
    g_tss_keys[key - 1].dtor = NULL;
    for (int i = 0; i < RXDK_TSS_SLOTS; ++i) {
        if (g_tss_slots[i].inuse && g_tss_slots[i].key == key)
            g_tss_slots[i].inuse = 0;
    }
    reg_unlock();
}
