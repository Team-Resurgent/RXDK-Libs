#include "common.h"

static LPVOID g_mainFiber;
static DWORD g_fiberResult;

static void __stdcall fiber_entry(LPVOID param)
{
    const DWORD* seed = (const DWORD*)param;
    g_fiberResult = *seed + 1u;
    SwitchToFiber(g_mainFiber);
}

int test_fiber(void)
{
    g_fiberResult = 0;

    g_mainFiber = ConvertThreadToFiber(NULL);
    if (!g_mainFiber) {
        return XAPI_SKIP;
    }

    DWORD seed = 41;
    LPVOID child = CreateFiber(0, fiber_entry, &seed);
    if (!child) {
        return 1;
    }

    SwitchToFiber(child);
    DeleteFiber(child);

    if (g_fiberResult != 42u) {
        return 2;
    }

    return XAPI_OK;
}
