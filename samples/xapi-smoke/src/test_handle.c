#include "common.h"

int test_handle(void)
{
    HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!event) {
        return 1;
    }

    HANDLE dup = NULL;
    if (!DuplicateHandle(
            GetCurrentProcess(),
            event,
            GetCurrentProcess(),
            &dup,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS)) {
        CloseHandle(event);
        return 2;
    }

    if (!SetEvent(event)) {
        CloseHandle(dup);
        CloseHandle(event);
        return 3;
    }

    if (WaitForSingleObject(dup, 0) != WAIT_OBJECT_0) {
        CloseHandle(dup);
        CloseHandle(event);
        return 4;
    }

    CloseHandle(dup);
    CloseHandle(event);
    return XAPI_OK;
}
