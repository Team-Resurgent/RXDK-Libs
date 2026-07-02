#include "common.h"

int test_vec3_transform_coord_identity(void)
{
    D3DXMATRIX ident;
    D3DXVECTOR3 v = { 3.0f, -4.0f, 5.0f };
    D3DXVECTOR3 out;

    D3DXMatrixIdentity(&ident);
    D3DXVec3TransformCoord(&out, &v, &ident);

    if (!xmath_approx_eq3(out.x, out.y, out.z, 3.0f, -4.0f, 5.0f, 0.0005f)) {
        xmath_trace_fail_f3("vec3_transform_coord_identity", 3.0f, -4.0f, 5.0f, out.x, out.y, out.z);
        return 1;
    }
    return XMATH_OK;
}

int test_vec3_transform_coord_translate(void)
{
    D3DXMATRIX t;
    D3DXVECTOR3 v = { 1.0f, 2.0f, 3.0f };
    D3DXVECTOR3 out;

    D3DXMatrixIdentity(&t);
    t._41 = 10.0f;
    t._42 = 20.0f;
    t._43 = 30.0f;

    D3DXVec3TransformCoord(&out, &v, &t);

    if (!xmath_approx_eq3(out.x, out.y, out.z, 11.0f, 22.0f, 33.0f, 0.0005f)) {
        xmath_trace_fail_f3("vec3_transform_coord_translate", 11.0f, 22.0f, 33.0f, out.x, out.y, out.z);
        return 1;
    }
    return XMATH_OK;
}
