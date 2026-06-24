/* Link smoke: libc + libxapi.lib + xboxkrnl.lib — resolves MulDiv from k32. */
#include "xbox/kernel.h"

#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef long LONG;
typedef int INT;

typedef unsigned long DWORD;

DWORD WINAPI GetTickCount(void);
INT WINAPI MulDiv(LONG nNumber, LONG nNumerator, LONG nDenominator);

int main(void)
{
    DWORD ticks = GetTickCount();
    INT scaled = MulDiv(100, 7, 3);

    DbgPrint("RXDK-LibsZig xapi-link OK ticks=%u muldiv=%d\n", (unsigned)ticks, (int)scaled);
    for (;;) {
    }
}
