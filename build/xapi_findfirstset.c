/* RXDK replacement for rtl/i386/bitmapa.asm (RtlFindFirstSetRightMember). */

#include <xboxkrnl/xboxdef.h>

#ifndef FASTCALL
#define FASTCALL __attribute__((fastcall))
#endif

UCHAR FASTCALL RtlFindFirstSetRightMember(ULONG Set)
{
    if (Set == 0) {
        return 0;
    }

    for (int i = 0; i < 32; i++) {
        if (Set & (1UL << i)) {
            return (UCHAR)i;
        }
    }
    return 0;
}
