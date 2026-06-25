/* Link smoke: libxapi.lib + xboxkrnl only (no libc.lib / picolibc objects). */
#include <xapi.h>

int main(void)
{
    DWORD ticks = GetTickCount();
    INT scaled = MulDiv(100, 7, 3);

    DbgPrint("RXDK-LibsZig xapi-standalone-link OK ticks=%u muldiv=%d\n", (unsigned)ticks, (int)scaled);
    for (;;) {
    }
}
