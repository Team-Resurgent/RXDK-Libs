#include <stdint.h>

uintptr_t __x86_tls_tcb_offset;

void __set_tcb(void *tcb)
{
    (void)tcb;
}
