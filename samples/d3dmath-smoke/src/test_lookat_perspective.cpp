#include "common.h"

// eye=(0,0,-6) looking at the origin with up=(0,1,0): the resulting basis is
// axis-aligned (X=(1,0,0), Y=(0,1,0), Z=(0,0,1)), so the view matrix should be
// identity-rotation with just a translation of +6 along Z (_43 = -dot(Z,eye) =
// -(0*0+0*0+1*-6) = 6). Hand-computed, matches the exact cube-demo repro case.
int test_lookat_identity_axes(void)
{
    D3DXMATRIX view;
    D3DXVECTOR3 eye = { 0.0f, 0.0f, -6.0f };
    D3DXVECTOR3 at  = { 0.0f, 0.0f,  0.0f };
    D3DXVECTOR3 up  = { 0.0f, 1.0f,  0.0f };

    D3DXMatrixLookAtLH(&view, &eye, &at, &up);

    if (!xmath_approx_eq3(view._11, view._22, view._33, 1.0f, 1.0f, 1.0f, 0.0005f) ||
        !xmath_approx_eq3(view._41, view._42, view._43, 0.0f, 0.0f, 6.0f, 0.0005f) ||
        !xmath_approx_eq(view._44, 1.0f, 0.0005f))
    {
        xmath_trace_fail_f3("lookat_identity_axes", 0.0f, 0.0f, 6.0f, view._41, view._42, view._43);
        return 1;
    }
    return XMATH_OK;
}

// fovy=PI/4, aspect=4/3, zn=1, zf=200 -- the exact params the cube demo uses.
// h = cot(fovy/2) = cos(22.5deg)/sin(22.5deg) = 2.414213562
// w = h/aspect = 1.810660172
// _33 = zf/(zf-zn) = 200/199 = 1.005025126, _34=1, _43=-_33*zn, _44=0
int test_perspective_known(void)
{
    D3DXMATRIX proj;
    D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4.0f, 4.0f / 3.0f, 1.0f, 200.0f);

    if (!xmath_approx_eq(proj._11, 1.810660172f, 0.001f) ||
        !xmath_approx_eq(proj._22, 2.414213562f, 0.001f) ||
        !xmath_approx_eq(proj._33, 1.005025126f, 0.001f) ||
        !xmath_approx_eq(proj._34, 1.0f, 0.0005f) ||
        !xmath_approx_eq(proj._43, -1.005025126f, 0.001f) ||
        !xmath_approx_eq(proj._44, 0.0f, 0.0005f))
    {
        xmath_trace_fail_f3("perspective_known", 1.810660172f, 2.414213562f, 1.005025126f,
                             proj._11, proj._22, proj._33);
        return 1;
    }
    return XMATH_OK;
}
