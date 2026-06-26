#include <assert.h>
#include <stdio.h>
#include <stdbit.h>

int main(void)
{
#if __STDC_VERSION__ >= 202311L
    unsigned x = 0x80000000u;
    assert(stdc_leading_zeros(x) == 0);
#endif
    printf("RXDK-LibsZig conformance-c23 OK\n");
    for (;;)
        ;
}
