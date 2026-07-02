#include "common.h"

int xmath_approx_eq(float a, float b, float eps)
{
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= eps;
}

int xmath_approx_eq3(float ax, float ay, float az, float bx, float by, float bz, float eps)
{
    return xmath_approx_eq(ax, bx, eps)
        && xmath_approx_eq(ay, by, eps)
        && xmath_approx_eq(az, bz, eps);
}

void xmath_trace(const char* msg)
{
    DbgPrint("d3dmath-smoke: %s\n", msg);
}

void xmath_trace_fail_f3(const char* test, float ex, float ey, float ez,
                                            float ax, float ay, float az)
{
    // DbgPrint is a kernel export (real xboxkrnl on hardware/xemu); NT-derived
    // kernels historically avoid the FPU in kernel mode, and this kernel's
    // DbgPrint formatter turns out not to support %f at all -- it doesn't
    // consume/advance the vararg, which re-prints the prior argument (visible
    // as the test-name string repeated where floats should be) and eventually
    // walks the va_list far enough off the stack to crash. Format via
    // picolibc's sprintf (real %f support, vfprintf_float.c) into a buffer
    // first, then hand DbgPrint a plain %s -- no floats ever cross into
    // DbgPrint's own format-string parsing.
    char buf[192];
    sprintf(buf, "d3dmath-smoke: FAILED %s expected=(%.4f, %.4f, %.4f) actual=(%.4f, %.4f, %.4f)\n",
            test, ex, ey, ez, ax, ay, az);
    DbgPrint("%s", buf);
}

void xmath_trace_matrix(const char* label, const D3DXMATRIX* m)
{
    char buf[256];
    sprintf(buf, "d3dmath-smoke: %s row0=(%.4f,%.4f,%.4f,%.4f) row1=(%.4f,%.4f,%.4f,%.4f)\n",
            label, m->_11, m->_12, m->_13, m->_14, m->_21, m->_22, m->_23, m->_24);
    DbgPrint("%s", buf);
    sprintf(buf, "d3dmath-smoke: %s row2=(%.4f,%.4f,%.4f,%.4f) row3=(%.4f,%.4f,%.4f,%.4f)\n",
            label, m->_31, m->_32, m->_33, m->_34, m->_41, m->_42, m->_43, m->_44);
    DbgPrint("%s", buf);
}

void xmath_set_matrix(D3DXMATRIX* m,
    float m11, float m12, float m13, float m14,
    float m21, float m22, float m23, float m24,
    float m31, float m32, float m33, float m34,
    float m41, float m42, float m43, float m44)
{
    m->_11 = m11; m->_12 = m12; m->_13 = m13; m->_14 = m14;
    m->_21 = m21; m->_22 = m22; m->_23 = m23; m->_24 = m24;
    m->_31 = m31; m->_32 = m32; m->_33 = m33; m->_34 = m34;
    m->_41 = m41; m->_42 = m42; m->_43 = m43; m->_44 = m44;
}

void xmath_build_known_pair(D3DXMATRIX* a, D3DXMATRIX* b)
{
    xmath_set_matrix(a,
        1.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    xmath_set_matrix(b,
        1.0f, 0.0f, 0.0f, 0.0f,
        3.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

int xmath_check_known_product(const D3DXMATRIX* r, const char* label)
{
    const float eps = 0.0005f;
    if (!xmath_approx_eq(r->_11, 7.0f, eps) || !xmath_approx_eq(r->_12, 2.0f, eps) ||
        !xmath_approx_eq(r->_21, 3.0f, eps) || !xmath_approx_eq(r->_22, 1.0f, eps) ||
        !xmath_approx_eq(r->_33, 1.0f, eps) || !xmath_approx_eq(r->_44, 1.0f, eps))
    {
        xmath_trace_fail_f3(label, 7.0f, 2.0f, 3.0f, r->_11, r->_12, r->_21);
        xmath_trace_matrix(label, r);
        return 1;
    }
    return XMATH_OK;
}
