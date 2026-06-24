#include "common.h"

int test_time(void)
{
    const DWORD t0 = GetTickCount();
    Sleep(10);
    const DWORD t1 = GetTickCount();
    if (t1 < t0) {
        return 1;
    }

    LARGE_INTEGER freq;
    LARGE_INTEGER c0;
    LARGE_INTEGER c1;
    if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
        return 2;
    }
    if (!QueryPerformanceCounter(&c0)) {
        return 3;
    }
    Sleep(1);
    if (!QueryPerformanceCounter(&c1)) {
        return 4;
    }
    if (c1.QuadPart < c0.QuadPart) {
        return 5;
    }

    SYSTEMTIME utc;
    SYSTEMTIME local;
    GetSystemTime(&utc);
    GetLocalTime(&local);
    if (utc.wYear < 2000 || local.wYear < 2000) {
        return 6;
    }

    return XAPI_OK;
}
