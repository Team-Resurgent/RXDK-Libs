#include "common.h"

int test_strings(void)
{
    static const char a[] = "abc";
    static const char b[] = "ABC";

    if (lstrlenA(a) != 3) {
        return 1;
    }

    if (lstrcmpA(a, "abc") != 0) {
        return 2;
    }

    if (lstrcmpiA(a, b) != 0) {
        return 3;
    }

    return XAPI_OK;
}
