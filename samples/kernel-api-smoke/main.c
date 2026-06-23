#include "xbox/kernel.h"
#include <stdio.h>

int main(void)
{
    LARGE_INTEGER time;

    KeQuerySystemTime(&time);
    printf("RXDK-LibsZig kernel-api-smoke OK ticks=%lu\n", (unsigned long)time.u.LowPart);
    for (;;)
        ;
}
