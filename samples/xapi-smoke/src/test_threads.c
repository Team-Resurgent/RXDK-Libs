#include "common.h"

static volatile DWORD g_thread_done;

static DWORD WINAPI thread_proc(LPVOID param)
{
    (void)param;
    g_thread_done = 42;
    return 0;
}

int test_threads(void)
{
    g_thread_done = 0;

    HANDLE thread = CreateThread(NULL, 0, thread_proc, NULL, 0, NULL);
    if (!thread) {
        return 1;
    }

    DWORD wait = WaitForSingleObject(thread, 5000);
    if (wait != WAIT_OBJECT_0) {
        CloseHandle(thread);
        return 2;
    }

    if (g_thread_done != 42) {
        CloseHandle(thread);
        return 3;
    }

    CloseHandle(thread);
    return XAPI_OK;
}
