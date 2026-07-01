#include "xbox/kernel.h"
#include "xbox/xbox.h"

#include <stdint.h>

typedef void (*xbox_ctor_fn)(void);

extern int main(void);

/* MinGW/lld: __CTOR_LIST__[0] is (uintptr_t)-1 or a count; entries follow. */
extern xbox_ctor_fn __CTOR_LIST__[];

static int xbox_ctors_done;
static int xbox_runtime_ready;

static void xbox_run_global_ctors(void)
{
    uintptr_t nptrs = (uintptr_t)(intptr_t)__CTOR_LIST__[0];

    if (nptrs == (uintptr_t)-1) {
        nptrs = 0;
        while (__CTOR_LIST__[nptrs + 1])
            nptrs++;
    }

    for (uintptr_t i = nptrs; i >= 1; i--)
        __CTOR_LIST__[i]();
}

void xbox_runtime_init(void)
{
    if (!xbox_runtime_ready) {
        xbox_runtime_ready = 1;
        DbgPrint("RXDK-Libs: runtime init\n");
    }
}

void xbox_halt(void)
{
    for (;;)
        ;
}

/*
 * The kernel invokes the XBE entry point on its Phase-1 init thread, which has
 * only a fixed KERNEL_STACK_SIZE (12 KiB) stack -- far too small for deep work
 * like the DWARF exception unwinder. Per the kernel contract (ntos init.c: the
 * entry "runs the main title thread on a new thread, so terminate this
 * initialization thread"), the XBE startup must spawn the real main() on its
 * own thread sized from the XBE header's SizeOfStackCommit, then return so the
 * kernel reclaims the init thread.
 *
 * The XBE image header is mapped at the image base (XBEIMAGE_STANDARD_BASE_ADDRESS
 * == our --image-base 0x10000). SizeOfStackCommit sits at offset 0x130
 * (Signature[4] + EncryptedDigest[256] + 8 ULONG/PVOID fields); this matches the
 * leaked XBEIMAGE_HEADER and RXDK-Tools XbeImageReader. Reading it here keeps libc
 * kernel-only (XeImageHeader()/xbeimage.h are libxapi-side headers).
 */
#define XBE_IMAGE_BASE         0x00010000u
#define XBE_SIZEOF_STACK_OFF   0x130u

static VOID NTAPI xbox_main_start(PVOID ctx)
{
    (void)ctx;
    (void)main();
}

static VOID NTAPI xbox_main_system_routine(PKSTART_ROUTINE start, PVOID ctx)
{
    start(ctx);
    PsTerminateSystemThread(0);
}

void xbox_start_main(void)
{
    HANDLE h;
    NTSTATUS s;
    ULONG stack = *(const volatile ULONG *)(XBE_IMAGE_BASE + XBE_SIZEOF_STACK_OFF);

    /* Guard against an unset/garbage header value; 64 KiB is our default. */
    if (stack < 0x4000u)
        stack = 0x10000u;

    s = PsCreateSystemThreadEx(&h, 0, stack, 0, NULL, xbox_main_start, NULL,
                               FALSE, FALSE, xbox_main_system_routine);
    if (NT_SUCCESS(s)) {
        /* We don't join; main runs the title. Returning lets the kernel
           terminate this init thread. */
        NtClose(h);
    } else {
        /* Fall back to running inline on the small init stack. */
        (void)main();
    }
}

void __main(void)
{
    if (xbox_ctors_done)
        return;
    xbox_ctors_done = 1;
    xbox_run_global_ctors();
}
