#include "common.h"

// Identity world/view/proj, 2x2 viewport at the origin: no perspective divide
// happens (w==1), so NDC == input, and the viewport map is trivial to verify
// by hand: screen.x=(1+1)*0.5*2=2, screen.y=(-1+1)*0.5*2=0, screen.z=1.
int test_vec3_project_identity(void)
{
    D3DXMATRIX ident;
    D3DVIEWPORT8 vp;
    D3DXVECTOR3 v = { 1.0f, 1.0f, 1.0f };
    D3DXVECTOR3 out;

    D3DXMatrixIdentity(&ident);
    RtlZeroMemory(&vp, sizeof(vp));
    vp.Width = 2;
    vp.Height = 2;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    D3DXVec3Project(&out, &v, &vp, &ident, &ident, &ident);

    if (!xmath_approx_eq3(out.x, out.y, out.z, 2.0f, 0.0f, 1.0f, 0.001f)) {
        xmath_trace_fail_f3("vec3_project_identity", 2.0f, 0.0f, 1.0f, out.x, out.y, out.z);
        return 1;
    }
    return XMATH_OK;
}

// This is the EXACT reproduction of the d3d8-cube-lib blank-screen bug: eye=
// (0,0,-6) looking at the origin, fovy=PI/4, aspect=4/3, near=1, far=200,
// 640x480 viewport, projecting the cube corner (-1,-1,-1) with WORLD=identity
// (pWorld=NULL, so D3DXVec3Project's case 3 combines VIEW*PROJECTION).
//
// Hand-computed (see session notes): view*proj row2 = [0,0,1.005025126,1],
// row3 = [0,0,5.025125628,6]; transforming (-1,-1,-1) gives clip=
// (-1.810660172, -2.414213562, 4.020100502), w=5; NDC=(-0.362132034,
// -0.482842712, 0.804020100); viewport-mapped screen ~= (204.12, 355.88,
// 0.8040). On real hardware / xemu, this was observed to actually come out as
// (320.00, 240.00, 0.0000) for every one of the cube's 8 corners regardless of
// position -- i.e. the transform was collapsing to the screen center instead
// of this. If this test fails with (320, 240, 0)-ish output again, the bug is
// still present.
int test_vec3_project_cube_corner(void)
{
    D3DXMATRIX proj, view;
    D3DVIEWPORT8 vp;
    D3DXVECTOR3 eye = { 0.0f, 0.0f, -6.0f };
    D3DXVECTOR3 at  = { 0.0f, 0.0f,  0.0f };
    D3DXVECTOR3 up  = { 0.0f, 1.0f,  0.0f };
    D3DXVECTOR3 corner = { -1.0f, -1.0f, -1.0f };
    D3DXVECTOR3 out;

    D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4.0f, 4.0f / 3.0f, 1.0f, 200.0f);
    D3DXMatrixLookAtLH(&view, &eye, &at, &up);

    RtlZeroMemory(&vp, sizeof(vp));
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    D3DXVec3Project(&out, &corner, &vp, &proj, &view, NULL);

    // Wide-ish tolerance (float trig + a hand-rounded expected value), but
    // tight enough that landing on (320, 240) -- the degenerate bug result --
    // would still fail loudly.
    if (!xmath_approx_eq3(out.x, out.y, out.z, 204.12f, 355.88f, 0.8040f, 1.0f)) {
        xmath_trace_fail_f3("vec3_project_cube_corner", 204.12f, 355.88f, 0.8040f, out.x, out.y, out.z);
        return 1;
    }
    return XMATH_OK;
}
