/*
 * malloc family on the Xbox process heap.
 *
 * Replaces picolibc's sbrk chunk allocator (libc/stdlib/malloc.c and friends,
 * excluded in libs/libc/build.zig). The reason is alignment, not speed.
 *
 * XDK math and graphics code loads matrices and vectors with SSE `movaps`,
 * which requires a 16-byte-aligned operand and raises #GP otherwise -- on this
 * kernel that surfaces as an access violation reporting a faulting address of
 * zero, which is a thoroughly misleading way to say "misaligned". The console
 * satisfies the requirement through the heap: NT rounds every block up to
 * HEAP_GRANULARITY == sizeof(HEAP_ENTRY) and hands back BusyBlock + 1, and
 * HEAP_ENTRY carries an extra ULONGLONG under _XBOX (libxapi's
 * support/inc/ntos/heap.h), so the granularity is 16 and every user pointer is
 * 16-aligned. picolibc instead promises only _Alignof(max_align_t), which is 8
 * here, so a __declspec(align(16)) structure reached through `new` could land
 * on any 4-byte boundary -- that is what made XGMatrixMultiply fault on its
 * first argument load.
 *
 * Routing malloc at the process heap is also what the retail XDK's CRT did, so
 * a title's malloc, HeapAlloc, LocalAlloc and operator new all share one heap
 * and one set of guarantees.
 *
 * Layering: unlike the rest of libs/libc/xbox, this file reaches into libxapi
 * (RtlAllocateHeap, XapiProcessHeap) rather than the kernel, because RXDK
 * implements the RTL heap there. A program that links libc without libxapi and
 * calls malloc will fail to link, which is the intended signal: it needs
 * libxapi_core.lib. XapiInitProcess creates the process heap before the CRT
 * runs, but malloc still creates one on demand so the `start`-entry and DXT
 * paths (which never call XapiInitProcess) work.
 */

#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xbox/kernel.h"

/* libxapi rtl/heap.c; declared here so libc need not pull an internal header. */
PVOID __stdcall RtlCreateHeap(ULONG Flags, PVOID HeapBase, SIZE_T ReserveSize,
                              SIZE_T CommitSize, PVOID Lock, PVOID Parameters);
PVOID __stdcall RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
PVOID __stdcall RtlReAllocateHeap(PVOID HeapHandle, ULONG Flags,
                                  PVOID BaseAddress, SIZE_T Size);
BOOLEAN __stdcall RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);
SIZE_T __stdcall RtlSizeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);

extern PVOID XapiProcessHeap;

#define HEAP_GROWABLE       0x00000002UL
#define HEAP_ZERO_MEMORY    0x00000008UL

/* What the heap guarantees, and therefore what plain malloc guarantees. */
#define HEAP_ALIGN          16u

/* Largest alignment memalign() honors; valloc()'s page alignment is the point. */
#define MAX_ALIGN           4096u

/*
 * Over-aligned blocks -- memalign/aligned_alloc/posix_memalign/valloc/pvalloc
 * asking for more than the heap already gives -- are a padded allocation with
 * the user pointer moved forward, so free() has to recover the real base and
 * realloc() has to reproduce the alignment. Three words below the user pointer
 * carry that: the distance back to the base at p[-16], a tag at p[-12], and the
 * alignment the caller asked for at p[-8]. The distance is not itself the
 * alignment -- the base is only 16-aligned, so the padding needed lands
 * anywhere in [16, align] and is usually not even a power of two.
 *
 * The first two words overlay HEAP_ENTRY for an ordinary block, and p[-12] in
 * particular spans its SegmentIndex/Flags/UnusedBytes/SmallTagIndex bytes. The
 * tag's second byte is deliberately zero, and that is the byte that lines up
 * with Flags, which the heap always stamps with HEAP_ENTRY_BUSY on an allocated
 * block. So an ordinary block can never read back as tagged: the test is exact,
 * not merely improbable, and free() never has to write to heap metadata to keep
 * it that way.
 */
#define ALIGN_TAG           0x584700A1u

static inline uint32_t *tag_slot(void *p)
{
    return (uint32_t *)((char *)p - 12);
}

static inline uint32_t *delta_slot(void *p)
{
    return (uint32_t *)((char *)p - 16);
}

static inline uint32_t *align_slot(void *p)
{
    return (uint32_t *)((char *)p - 8);
}

/* Distance from a user pointer back to its heap block, 0 if not over-aligned. */
static uint32_t block_offset(void *p)
{
    uint32_t delta;

    if (*tag_slot(p) != ALIGN_TAG)
        return 0;

    delta = *delta_slot(p);
    if (delta < HEAP_ALIGN || delta > MAX_ALIGN || (delta % HEAP_ALIGN) != 0)
        return 0;

    return delta;
}

static void *block_base(void *p)
{
    return (char *)p - block_offset(p);
}

static PVOID process_heap(void)
{
    PVOID heap = XapiProcessHeap;
    PVOID created;
    PVOID won;

    if (heap != NULL)
        return heap;

    /*
     * No XapiInitProcess in this image (a DXT, or a `start`-entry program), so
     * stand the process heap up here and publish it, keeping malloc and
     * HeapAlloc/LocalAlloc on one heap as they are in a normal title.
     */
    created = RtlCreateHeap(HEAP_GROWABLE, NULL, 0, 0, NULL, NULL);
    if (created == NULL)
        return NULL;

    won = (PVOID)(uintptr_t)InterlockedCompareExchange(
        (LONG volatile *)&XapiProcessHeap, (LONG)(uintptr_t)created, 0);
    if (won != NULL)
        return won; /* lost the race; the winner's heap is the process heap */

    return created;
}

static void *heap_alloc(size_t size, ULONG flags)
{
    PVOID heap = process_heap();
    void *p;

    if (heap == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    p = RtlAllocateHeap(heap, flags, size);
    if (p == NULL)
        errno = ENOMEM;

    return p;
}

void *malloc(size_t size)
{
    return heap_alloc(size, 0);
}

void free(void *p)
{
    PVOID heap;

    if (p == NULL)
        return;

    heap = XapiProcessHeap;
    if (heap == NULL)
        return; /* nothing was ever allocated, so nothing can be freed */

    RtlFreeHeap(heap, 0, block_base(p));
}

void cfree(void *p)
{
    free(p);
}

void *calloc(size_t n, size_t elem)
{
    if (n != 0 && elem > (size_t)-1 / n) {
        errno = ENOMEM;
        return NULL;
    }

    return heap_alloc(n * elem, HEAP_ZERO_MEMORY);
}

size_t malloc_usable_size(void *p)
{
    PVOID heap;
    uint32_t delta;
    SIZE_T size;

    if (p == NULL)
        return 0;

    heap = XapiProcessHeap;
    if (heap == NULL)
        return 0;

    delta = block_offset(p);
    size = RtlSizeHeap(heap, 0, (char *)p - delta);
    if (size == (SIZE_T)-1)
        return 0;

    return (size_t)size - delta;
}

void *realloc(void *p, size_t size)
{
    PVOID heap;
    uint32_t delta;
    void *n;

    if (p == NULL)
        return malloc(size);

    if (size == 0) {
        free(p);
        return NULL;
    }

    heap = process_heap();
    if (heap == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    /*
     * An over-aligned block can't grow in place: the heap would keep the base
     * where it is, and nothing then preserves the padding that made the user
     * pointer aligned. Move it into a fresh block of the same alignment.
     */
    delta = block_offset(p);
    if (delta != 0) {
        size_t old = malloc_usable_size(p);

        n = memalign(*align_slot(p), size);
        if (n == NULL)
            return NULL;
        memcpy(n, p, old < size ? old : size);
        free(p);
        return n;
    }

    n = RtlReAllocateHeap(heap, 0, p, size);
    if (n == NULL)
        errno = ENOMEM;

    return n;
}

void *reallocarray(void *p, size_t n, size_t elem)
{
    if (n != 0 && elem > (size_t)-1 / n) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(p, n * elem);
}

void *reallocf(void *p, size_t size)
{
    void *n = realloc(p, size);

    if (n == NULL && size != 0)
        free(p);
    return n;
}

void *memalign(size_t align, size_t size)
{
    void *base;
    void *p;
    size_t delta;

    if (align == 0 || (align & (align - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }

    if (align <= HEAP_ALIGN)
        return malloc(size);

    if (align > MAX_ALIGN || size > (size_t)-1 - align) {
        errno = ENOMEM;
        return NULL;
    }

    /*
     * Pad by a whole alignment rather than align - HEAP_ALIGN: the base is only
     * 16-aligned, so the delta ranges over [16, align], and the tag words need
     * the full 16 bytes below the user pointer to be ours.
     */
    base = malloc(size + align);
    if (base == NULL)
        return NULL;

    delta = align - ((uintptr_t)base & (align - 1));
    p = (char *)base + delta;

    *delta_slot(p) = (uint32_t)delta;
    *tag_slot(p) = ALIGN_TAG;
    *align_slot(p) = (uint32_t)align;
    return p;
}

void *aligned_alloc(size_t align, size_t size)
{
    return memalign(align, size);
}

int posix_memalign(void **memptr, size_t align, size_t size)
{
    void *p;

    if (align == 0 || (align & (align - 1)) != 0 || align % sizeof(void *) != 0)
        return EINVAL;

    p = memalign(align, size);
    if (p == NULL)
        return ENOMEM;

    *memptr = p;
    return 0;
}

void *valloc(size_t size)
{
    return memalign(MAX_ALIGN, size);
}

void *pvalloc(size_t size)
{
    if (size > (size_t)-1 - (MAX_ALIGN - 1)) {
        errno = ENOMEM;
        return NULL;
    }
    return valloc((size + MAX_ALIGN - 1) & ~(size_t)(MAX_ALIGN - 1));
}
