#include "common.h"

//
// Contended critical-section stress.
//
// test_sync only exercises the uncontended path (a single thread entering and
// leaving), which never makes a thread wait. This test forces real contention:
// several threads hammer one critical section at once, so acquires actually
// block and the wait/wake path is used.
//
// A lock that wedges under contention shows up here as a thread that never
// finishes -- the waits below are bounded so the test reports a failure code
// instead of hanging the run.
//

#define CS_THREADS      8
#define CS_ITERATIONS   2000
#define CS_WAIT_MS      15000

static CRITICAL_SECTION g_cs;
static volatile LONG    g_counter;      /* guarded by g_cs, deliberately non-atomic */
static volatile LONG    g_inside;       /* how many threads are in the section */
static volatile LONG    g_max_inside;   /* must never exceed 1 */

static DWORD __stdcall cs_thread(LPVOID param)
{
    int i;

    (void)param;

    for (i = 0; i < CS_ITERATIONS; i++) {
        EnterCriticalSection(&g_cs);

        if (InterlockedIncrement((LONG *)&g_inside) > g_max_inside) {
            g_max_inside = g_inside;
        }

        /* Non-atomic on purpose: only mutual exclusion can keep this correct. */
        g_counter = g_counter + 1;

        InterlockedDecrement((LONG *)&g_inside);

        LeaveCriticalSection(&g_cs);
    }

    return 0;
}

int test_cs_stress(void)
{
    HANDLE threads[CS_THREADS];
    int    created = 0;
    int    i;
    int    result = XAPI_OK;

    g_counter    = 0;
    g_inside     = 0;
    g_max_inside = 0;

    InitializeCriticalSection(&g_cs);

    /* Recursive acquisition by the owning thread must be allowed and balanced. */
    EnterCriticalSection(&g_cs);
    EnterCriticalSection(&g_cs);
    LeaveCriticalSection(&g_cs);
    LeaveCriticalSection(&g_cs);

    for (i = 0; i < CS_THREADS; i++) {
        threads[i] = CreateThread(NULL, 0, cs_thread, NULL, 0, NULL);
        if (!threads[i]) {
            result = 1;
            break;
        }
        created++;
    }

    for (i = 0; i < created; i++) {
        if (WaitForSingleObject(threads[i], CS_WAIT_MS) != WAIT_OBJECT_0) {
            /* Thread never got through its loop: the lock stopped handing out
               ownership, which is the failure this test exists to catch. */
            if (result == XAPI_OK) {
                result = 10 + i;
            }
        }
        CloseHandle(threads[i]);
    }

    DeleteCriticalSection(&g_cs);

    if (result != XAPI_OK) {
        return result;
    }

    if (g_max_inside != 1) {
        return 2; /* mutual exclusion violated */
    }

    if (g_counter != (LONG)(CS_THREADS * CS_ITERATIONS)) {
        return 3; /* lost updates */
    }

    return XAPI_OK;
}
