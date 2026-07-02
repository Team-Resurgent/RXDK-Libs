#include "common.h"

// MatrixProduct4x4's SSE asm uses movaps, which faults (STATUS_ACCESS_VIOLATION)
// on a non-16-byte-aligned operand. The real call sites (lazy.cpp's
// LazySetTransform, device.hpp's CDevice::m_Transform/m_ProjectionViewportTransform)
// all use __declspec(align(16)) locals/members -- nothing in MatrixProduct4x4's
// own signature (plain D3DMATRIX*) enforces or documents that requirement, so
// replicate it explicitly here rather than relying on the compiler happening to
// align a plain stack D3DXMATRIX (it doesn't, by default -- confirmed the hard
// way: this crashed with an unaligned local on real hardware/xemu).
#define D3D_ALIGN16 __declspec(align(16))

// RxdkTestMatrixProduct4x4 wraps D3D::MatrixProduct4x4 (libs/libd3d8/se/math.cpp),
// the SSE-based function lazy.cpp's LazySetTransform actually calls to combine
// WORLD*VIEW and modelView*ProjectionViewport for the GPU's MODEL_VIEW/
// COMPOSITE transform registers -- i.e. the function that matters for whether
// anything actually shows up on screen, as opposed to the public D3DX8 API.
int test_matrix_product4x4_known(void)
{
    D3D_ALIGN16 D3DXMATRIX a, b, out;
    xmath_build_known_pair(&a, &b);
    RxdkTestMatrixProduct4x4((D3DMATRIX*)&out, (D3DMATRIX*)&a, (D3DMATRIX*)&b);
    return xmath_check_known_product(&out, "matrix_product4x4_known");
}

// Direct cross-check: D3DXMatrixMultiply (libd3dx8, x87 FPU asm) and
// MatrixProduct4x4 (libd3d8, SSE asm) both compute a plain 4x4 row-major
// matrix product -- for the same inputs they must produce the same output.
// If they disagree, exactly one of the two asm implementations is wrong.
int test_matrix_multiply_vs_product4x4(void)
{
    D3D_ALIGN16 D3DXMATRIX a, b, viaD3DX, viaSSE;
    xmath_build_known_pair(&a, &b);

    D3DXMatrixMultiply(&viaD3DX, &a, &b);
    RxdkTestMatrixProduct4x4((D3DMATRIX*)&viaSSE, (D3DMATRIX*)&a, (D3DMATRIX*)&b);

    if (!xmath_approx_eq3(viaD3DX._11, viaD3DX._12, viaD3DX._21,
                           viaSSE._11, viaSSE._12, viaSSE._21, 0.0005f) ||
        !xmath_approx_eq3(viaD3DX._22, viaD3DX._33, viaD3DX._44,
                           viaSSE._22, viaSSE._33, viaSSE._44, 0.0005f))
    {
        xmath_trace_fail_f3("matrix_multiply_vs_product4x4 (row0/row1)",
                             viaD3DX._11, viaD3DX._12, viaD3DX._21,
                             viaSSE._11, viaSSE._12, viaSSE._21);
        xmath_trace_matrix("matrix_multiply_vs_product4x4 viaD3DX", &viaD3DX);
        xmath_trace_matrix("matrix_multiply_vs_product4x4 viaSSE", &viaSSE);
        return 1;
    }
    return XMATH_OK;
}
