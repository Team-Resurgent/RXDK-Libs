#include "common.h"

int test_interlocked(void)
{
    LONG value = 10;

    if (InterlockedIncrement(&value) != 11 || value != 11) {
        return 1;
    }

    if (InterlockedDecrement(&value) != 10 || value != 10) {
        return 2;
    }

    const LONG exchanged = InterlockedCompareExchange(&value, 99, 10);
    if (exchanged != 10 || value != 99) {
        return 3;
    }

    return XAPI_OK;
}
