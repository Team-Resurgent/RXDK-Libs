/* Link smoke: libc + libxapi.lib + xboxkrnl.lib — resolves MulDiv from k32. */
#include <xapi.h>

int main(void)
{
    DWORD ticks = GetTickCount();
    INT scaled = MulDiv(100, 7, 3);

    DbgPrint("RXDK-LibsZig xapi-link OK ticks=%u muldiv=%d\n", (unsigned)ticks, (int)scaled);
    for (;;) {
    }
}
