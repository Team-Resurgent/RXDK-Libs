#include "common.h"

int test_widechar(void)
{
    static const char kAscii[] = "xapi-smoke";

    WCHAR wide[16];
    const int wideLen = MultiByteToWideChar(CP_ACP, 0, kAscii, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    if (wideLen <= 0) {
        return 1;
    }

    char roundTrip[16];
    const int asciiLen = WideCharToMultiByte(
        CP_ACP,
        0,
        wide,
        -1,
        roundTrip,
        (int)sizeof(roundTrip),
        NULL,
        NULL);
    if (asciiLen <= 0) {
        return 2;
    }

    if (lstrcmpA(roundTrip, kAscii) != 0) {
        return 3;
    }

    return XAPI_OK;
}
