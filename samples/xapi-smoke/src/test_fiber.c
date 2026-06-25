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

    xapi_smoke_trace_line("fiber ConvertThreadToFiber");
    g_mainFiber = ConvertThreadToFiber(NULL);
    if (!g_mainFiber) {
        return XAPI_SKIP;
    }

    DWORD seed = 41;
    xapi_smoke_trace_line("fiber CreateFiber");
    LPVOID child = CreateFiber(0, fiber_entry, &seed);
    if (!child) {
        return 1;
    }

    xapi_smoke_trace_line("fiber SwitchToFiber child");
    SwitchToFiber(child);
    xapi_smoke_trace_line("fiber DeleteFiber");
    DeleteFiber(child);

    if (g_fiberResult != 42u) {
        return 2;
    }

    return XAPI_OK;
}
