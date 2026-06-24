// xAPI category smoke — exercises major winbase.h / xbox.h areas on kit hardware.
// USB OHCD internals and exhaustive WINAPI coverage are out of scope.
#include "common.h"
#include "manifest.h"

static void hang_forever(void)
{
    for (;;) {
        Sleep(1000);
    }
}

static void fail_test(const char* name, int code)
{
    xapi_smoke_trace_fail(name, (unsigned)(code < 0 ? -code : code));
    hang_forever();
}

int main(void)
{
    unsigned skipped = 0;
    unsigned passed = 0;

    xapi_smoke_trace_line("begin");

    for (unsigned i = 0; i < kXapiSmokeTestCount; ++i) {
        const XapiSmokeTest* t = &kXapiSmokeTests[i];
        xapi_smoke_trace_line(t->name);

        const int rc = t->run();
        if (rc == XAPI_SKIP) {
            xapi_smoke_trace_line2("SKIP ", t->name);
            skipped++;
            continue;
        }
        if (rc != XAPI_OK) {
            fail_test(t->name, rc);
        }
        passed++;
    }

    xapi_smoke_trace_count("passed", passed);
    xapi_smoke_trace_count("skipped", skipped);
    xapi_smoke_trace_line("all runnable tests passed");
    hang_forever();
    return 0;
}
