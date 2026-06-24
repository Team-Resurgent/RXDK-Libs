#include "common.h"

int test_sync(void)
{
    HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!event) {
        return 1;
    }

    if (!SetEvent(event)) {
        CloseHandle(event);
        return 2;
    }

    DWORD wait = WaitForSingleObject(event, 0);
    if (wait != WAIT_OBJECT_0) {
        CloseHandle(event);
        return 3;
    }

    CloseHandle(event);

    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);
    EnterCriticalSection(&cs);
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);

    return XAPI_OK;
}
