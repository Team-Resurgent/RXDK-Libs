#include "common.h"

int test_matrix_multiply_identity(void)
{
    D3DXMATRIX ident, m, out;

    D3DXMatrixIdentity(&ident);
    xmath_set_matrix(&m,
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f);

    D3DXMatrixMultiply(&out, &m, &ident);

    if (!xmath_approx_eq3(out._11, out._12, out._13, 1.0f, 2.0f, 3.0f, 0.0005f) ||
        !xmath_approx_eq3(out._41, out._42, out._43, 13.0f, 14.0f, 15.0f, 0.0005f))
    {
        xmath_trace_fail_f3("matrix_multiply_identity", 1.0f, 2.0f, 3.0f, out._11, out._12, out._13);
        xmath_trace_matrix("matrix_multiply_identity", &out);
        return 1;
    }
    return XMATH_OK;
}

int test_matrix_multiply_known(void)
{
    D3DXMATRIX a, b, out;
    xmath_build_known_pair(&a, &b);
    D3DXMatrixMultiply(&out, &a, &b);
    return xmath_check_known_product(&out, "matrix_multiply_known");
}

// D3DXVec3Project's own internal usage (case 7, WVP) does
// D3DXMatrixMultiply(&mat, pWorld, pView); D3DXMatrixMultiply(&mat, &mat, pProjection);
// i.e. pOut aliases pM1 (pM2 stays distinct). Per the dispatch at the top of
// D3DXMatrixMultiply (`if (pM2 != pOut) goto LRowByColumn`), this still takes
// the LRowByColumn path -- ecx (=pM1=pOut) and ebx (=pOut) advance in lockstep
// a row at a time, so each row is fully read before it's overwritten. This is
// the actual pattern the real render/projection code exercises.
int test_matrix_multiply_aliased_pM1(void)
{
    D3DXMATRIX a, b, result;
    xmath_build_known_pair(&a, &b);
    result = a;
    D3DXMatrixMultiply(&result, &result, &b);
    return xmath_check_known_product(&result, "matrix_multiply_aliased_pM1");
}

// pOut aliasing pM2 (pM1 distinct) takes the *other* branch, LColumnByRow --
// a genuinely different asm routine from the one above, so it needs its own
// coverage rather than assuming symmetry with the pM1-aliased case.
int test_matrix_multiply_aliased_pM2(void)
{
    D3DXMATRIX a, b, result;
    xmath_build_known_pair(&a, &b);
    result = b;
    D3DXMatrixMultiply(&result, &a, &result);
    return xmath_check_known_product(&result, "matrix_multiply_aliased_pM2");
}
