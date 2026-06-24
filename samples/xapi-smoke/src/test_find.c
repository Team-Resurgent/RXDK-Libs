#include "common.h"

int test_find(void)
{
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFileA(XAPI_SMOKE_FIND_ALL, &data);
    if (find == INVALID_HANDLE_VALUE) {
        return 1;
    }

    unsigned count = 0;
    do {
        count++;
    } while (FindNextFileA(find, &data));

    FindClose(find);

    if (count == 0) {
        return 2;
    }

    return XAPI_OK;
}
