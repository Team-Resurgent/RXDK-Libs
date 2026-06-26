#include "xbox/kernel.h"
#include "xbox/xbox.h"

#include <stdint.h>

typedef void (*xbox_ctor_fn)(void);

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
        xbox_zero_uninitialized_data();
        xbox_runtime_ready = 1;
        DbgPrint("RXDK-LibsZig: runtime init\n");
    }
}

void xbox_halt(void)
{
    for (;;)
        ;
}

void __main(void)
{
    if (xbox_ctors_done)
        return;
    xbox_ctors_done = 1;
    xbox_run_global_ctors();
}
