#include "common.h"

// diag(2,4,5,1) is trivial to invert by hand: diag(0.5, 0.25, 0.2, 1).
static void build_diagonal(D3DXMATRIX* m)
{
    xmath_set_matrix(m,
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 4.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

static int check_diagonal_inverse(const D3DXMATRIX* r, const char* label)
{
    const float eps = 0.0005f;
    if (!xmath_approx_eq(r->_11, 0.5f, eps) || !xmath_approx_eq(r->_22, 0.25f, eps) ||
        !xmath_approx_eq(r->_33, 0.2f, eps) || !xmath_approx_eq(r->_44, 1.0f, eps) ||
        !xmath_approx_eq(r->_12, 0.0f, eps) || !xmath_approx_eq(r->_21, 0.0f, eps))
    {
        xmath_trace_fail_f3(label, 0.5f, 0.25f, 0.2f, r->_11, r->_22, r->_33);
        xmath_trace_matrix(label, r);
        return 1;
    }
    return XMATH_OK;
}

int test_matrix_inverse_diagonal(void)
{
    D3DXMATRIX m, inv;
    float det = 0.0f;
    build_diagonal(&m);
    D3DXMatrixInverse(&inv, &det, &m);

    // det(diag(2,4,5,1)) = 40
    if (!xmath_approx_eq(det, 40.0f, 0.01f)) {
        xmath_trace_fail_f3("matrix_inverse_diagonal (det)", 40.0f, 0.0f, 0.0f, det, 0.0f, 0.0f);
        return 1;
    }
    return check_diagonal_inverse(&inv, "matrix_inverse_diagonal");
}

// Inverse4x4 (libs/libd3d8/se/math.cpp) is the render path's own inverse --
// LazySetTransform calls it for the inverse modelview whenever lighting or
// texgen needs it. Same known diagonal matrix, called through the
// RxdkTestInverse4x4 wrapper with bNormalize=TRUE (the LazySetTransform call
// site always normalizes -- see Inverse4x4(&inverseModelView, &modelView,
// !D3D__RenderState[D3DRS_NORMALIZENORMALS]), the common case).
int test_inverse4x4_diagonal(void)
{
    D3DXMATRIX m, inv;
    build_diagonal(&m);

    int rc = RxdkTestInverse4x4((D3DMATRIX*)&inv, (D3DMATRIX*)&m, TRUE);
    if (rc != 0) {
        xmath_trace_fail_f3("inverse4x4_diagonal (rc)", 0.0f, 0.0f, 0.0f, (float)rc, 0.0f, 0.0f);
        return 1;
    }
    return check_diagonal_inverse(&inv, "inverse4x4_diagonal");
}
