// D3D8/D3DX8 matrix math smoke test -- exercises D3DXMatrixMultiply,
// D3DXVec3TransformCoord, D3DXVec3Project, D3DXMatrixLookAtLH,
// D3DXMatrixPerspectiveFovLH (libd3dx8) plus MatrixProduct4x4 (libd3d8, the
// SSE-based function the real render pipeline actually uses to combine
// WORLD/VIEW/PROJECTION -- see lazy.cpp's LazySetTransform) against
// hand-computed expected values, on kit hardware / xemu.
#include "common.h"
#include "manifest.h"

static void hang_forever(void)
{
    for (;;) {
        Sleep(1000);
    }
}

int main(void)
{
    unsigned skipped = 0;
    unsigned passed = 0;
    unsigned failed = 0;

    xmath_trace("begin");

    for (unsigned i = 0; i < kD3DMathSmokeTestCount; ++i) {
        const D3DMathSmokeTest* t = &kD3DMathSmokeTests[i];
        DbgPrint("d3dmath-smoke: %s\n", t->name);

        const int rc = t->run();
        if (rc == XMATH_SKIP) {
            DbgPrint("d3dmath-smoke: SKIP %s\n", t->name);
            skipped++;
            continue;
        }
        if (rc != XMATH_OK) {
            // The individual test already traced the expected/actual values
            // via xmath_trace_fail_f3 before returning non-zero.
            failed++;
            continue;
        }
        passed++;
    }

    DbgPrint("d3dmath-smoke: passed n=%u\n", passed);
    DbgPrint("d3dmath-smoke: failed n=%u\n", failed);
    DbgPrint("d3dmath-smoke: skipped n=%u\n", skipped);
    xmath_trace(failed == 0 ? "all tests passed" : "SOME TESTS FAILED");
    hang_forever();
    return 0;
}
