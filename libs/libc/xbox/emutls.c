/*
 * Emulated-TLS runtime + C++ thread_local destructor registration.
 *
 * clang -femulated-tls lowers `thread_local` to __emutls_get_address() over a
 * per-thread table, instead of the Windows TLS model (__tls_index + a per-thread
 * TLS block via the TEB) which raw kernel threads here don't set up. We provide
 * that runtime, plus the Itanium __cxa_thread_atexit (libc++abi's is compiled out
 * by _LIBCXXABI_HAS_NO_THREADS). Both hang off C11 tss keys, whose destructors run
 * at thread exit (see threads.c rxdk_run_tss_dtors), so thread_local objects with
 * non-trivial destructors are destructed and their storage reclaimed.
 *
 * Limits: at most EMUTLS_MAX thread_local objects touched per thread, and object
 * alignment up to malloc's guarantee (over-aligned thread_locals are unsupported).
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define EMUTLS_MAX 256

/* clang/compiler-rt ABI: one of these per thread_local variable. */
typedef struct __emutls_control {
    size_t size;
    size_t align;
    union {
        uintptr_t index; /* data[index-1] is this thread's instance */
        void *address;
    } object;
    void *value; /* initializer template, or NULL for zero-init */
} __emutls_control;

/* Per-thread destructor list (LIFO), run at thread exit. */
struct tl_dtor {
    void (*dtor)(void *);
    void *obj;
    struct tl_dtor *next;
};

/* Per-thread emutls instance table. */
struct emutls_array {
    uintptr_t size;
    void *data[];
};

static tss_t g_tl_key;     /* head of the per-thread tl_dtor list */
static tss_t g_emutls_key; /* per-thread struct emutls_array* */
static once_flag g_once = ONCE_FLAG_INIT;
static mtx_t g_index_lock; /* guards g_index */
static uintptr_t g_index;  /* last assigned emutls object index (1-based) */

/* tss destructor for g_tl_key: run registered dtors LIFO. The emutls array free
   is itself registered here (first, so it runs last), after all object dtors. */
static void run_tl_dtors(void *p)
{
    struct tl_dtor *n = (struct tl_dtor *)p;
    while (n) {
        struct tl_dtor *next = n->next;
        n->dtor(n->obj);
        free(n);
        n = next;
    }
}

static void free_emutls_array(void *p)
{
    struct emutls_array *a = (struct emutls_array *)p;
    if (!a)
        return;
    for (uintptr_t i = 0; i < a->size; ++i)
        free(a->data[i]);
    free(a);
}

static void tls_init(void)
{
    tss_create(&g_tl_key, run_tl_dtors);
    tss_create(&g_emutls_key, NULL); /* array freed via the tl_dtor list */
    mtx_init(&g_index_lock, mtx_plain);
}

int __cxa_thread_atexit(void (*dtor)(void *), void *obj, void *dso)
{
    (void)dso;
    call_once(&g_once, tls_init);
    struct tl_dtor *n = (struct tl_dtor *)malloc(sizeof(*n));
    if (!n)
        return -1;
    n->dtor = dtor;
    n->obj = obj;
    n->next = (struct tl_dtor *)tss_get(g_tl_key);
    tss_set(g_tl_key, n);
    return 0;
}

void *__emutls_get_address(void *control)
{
    __emutls_control *c = (__emutls_control *)control;
    call_once(&g_once, tls_init);

    uintptr_t index = __atomic_load_n(&c->object.index, __ATOMIC_ACQUIRE);
    if (index == 0) {
        mtx_lock(&g_index_lock);
        if (c->object.index == 0)
            __atomic_store_n(&c->object.index, ++g_index, __ATOMIC_RELEASE);
        index = c->object.index;
        mtx_unlock(&g_index_lock);
    }

    struct emutls_array *a = (struct emutls_array *)tss_get(g_emutls_key);
    if (!a) {
        a = (struct emutls_array *)calloc(
            1, sizeof(*a) + (size_t)EMUTLS_MAX * sizeof(void *));
        if (!a)
            return NULL;
        a->size = EMUTLS_MAX;
        tss_set(g_emutls_key, a);
        /* Reclaim the table (and its objects) at thread exit. Registered before
           any object's dtor, so the LIFO list runs it last. */
        __cxa_thread_atexit(free_emutls_array, a, NULL);
    }
    if (index == 0 || index > a->size)
        return NULL; /* > EMUTLS_MAX thread_locals in one thread */

    void *obj = a->data[index - 1];
    if (!obj) {
        obj = malloc(c->size ? c->size : 1);
        if (!obj)
            return NULL;
        if (c->value)
            memcpy(obj, c->value, c->size);
        else
            memset(obj, 0, c->size);
        a->data[index - 1] = obj;
    }
    return obj;
}
