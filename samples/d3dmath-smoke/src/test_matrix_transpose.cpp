#include "common.h"

// M = [[1,2,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
// M^T rows = [M col0, M col1, M col2, M col3] = [[1,0,0,0],[2,1,0,0],[0,0,1,0],[0,0,0,1]]
static void build_known_matrix(D3DXMATRIX* m)
{
    xmath_set_matrix(m,
        1.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

static int check_transpose_result(const D3DXMATRIX* r, const char* label)
{
    const float eps = 0.0005f;
    if (!xmath_approx_eq(r->_11, 1.0f, eps) || !xmath_approx_eq(r->_12, 0.0f, eps) ||
        !xmath_approx_eq(r->_21, 2.0f, eps) || !xmath_approx_eq(r->_22, 1.0f, eps) ||
        !xmath_approx_eq(r->_33, 1.0f, eps) || !xmath_approx_eq(r->_44, 1.0f, eps))
    {
        xmath_trace_fail_f3(label, 1.0f, 0.0f, 2.0f, r->_11, r->_12, r->_21);
        xmath_trace_matrix(label, r);
        return 1;
    }
    return XMATH_OK;
}

int test_matrix_transpose_known(void)
{
    D3DXMATRIX m, out;
    build_known_matrix(&m);
    D3DXMatrixTranspose(&out, &m);
    return check_transpose_result(&out, "matrix_transpose_known");
}

int test_matrix_transpose_self_aliased(void)
{
    D3DXMATRIX m;
    build_known_matrix(&m);
    D3DXMatrixTranspose(&m, &m);
    return check_transpose_result(&m, "matrix_transpose_self_aliased");
}
