#include "common.h"

int test_sync2(void)
{
    HANDLE mutex = CreateMutexA(NULL, FALSE, NULL);
    if (!mutex) {
        return 1;
    }

    const DWORD mutexWait = WaitForSingleObject(mutex, 0);
    if (mutexWait == WAIT_FAILED) {
        CloseHandle(mutex);
        return 2;
    }
    if (!ReleaseMutex(mutex)) {
        CloseHandle(mutex);
        return 3;
    }
    CloseHandle(mutex);

    HANDLE sem = CreateSemaphoreA(NULL, 0, 1, NULL);
    if (!sem) {
        return 4;
    }
    if (!ReleaseSemaphore(sem, 1, NULL)) {
        CloseHandle(sem);
        return 5;
    }
    if (WaitForSingleObject(sem, 0) != WAIT_OBJECT_0) {
        CloseHandle(sem);
        return 6;
    }
    CloseHandle(sem);

    HANDLE e0 = CreateEventA(NULL, FALSE, FALSE, NULL);
    HANDLE e1 = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!e0 || !e1) {
        if (e0) {
            CloseHandle(e0);
        }
        if (e1) {
            CloseHandle(e1);
        }
        return 7;
    }

    SetEvent(e1);
    HANDLE handles[2] = { e0, e1 };
    const DWORD multi = WaitForMultipleObjects(2, handles, FALSE, 100);
    CloseHandle(e0);
    CloseHandle(e1);
    if (multi != WAIT_OBJECT_0 + 1) {
        return 8;
    }

    return XAPI_OK;
}
