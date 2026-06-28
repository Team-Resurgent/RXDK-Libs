//------------------------------------------------------------------------------
// Direct3D 8 rotating colored triangle -- RXDK libd3d8 sample.
//
// Creates a 640x480 HAL device with a Z-buffer and spins a colored triangle via
// the world transform. Uses the Xbox D3D8 C API (Direct3D_* / D3DDevice_* global
// entry points) directly -- no D3DX8 (that library isn't ported yet), so the
// view/projection/rotation matrices are built by hand here.
//------------------------------------------------------------------------------

// common.h must come first: it pulls the xapi/xtl/NT environment (defining
// NT_INCLUDED + the xboxkrnl base types) so the later windows/d3d8 headers use
// our types and skip zig's MinGW <winnt.h>.
#include "common.h"   // xapi boot/trace helpers (shared with the xapi samples)
#include <stdio.h>
#include <math.h>
#include <guiddef.h>  // GUID/REFGUID -- d3d8.h's resource interfaces reference them
#include <d3d8.h>

#define TRI_PI 3.14159265358979323846f

typedef struct {
    float x, y, z;
    DWORD color;
} CustomVertex;

#define TRI_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

static const CustomVertex g_triangle[3] = {
    {  0.0f, -1.1547f, 0.0f, 0xffffff00 },   // bottom  - yellow
    { -1.0f,  0.5777f, 0.0f, 0xff00ff00 },   // top-left - green
    {  1.0f,  0.5777f, 0.0f, 0xffff0000 },   // top-right - red
};

//--- hand-rolled matrix math (left-handed, D3D row-vector convention) ----------

static void mat_identity(D3DMATRIX *m)
{
    RtlZeroMemory(m, sizeof(*m));
    m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
}

static void mat_perspective_fov_lh(D3DMATRIX *m, float fovY, float aspect,
                                    float zn, float zf)
{
    float h = 1.0f / tanf(fovY * 0.5f);
    float w = h / aspect;
    RtlZeroMemory(m, sizeof(*m));
    m->_11 = w;
    m->_22 = h;
    m->_33 = zf / (zf - zn);
    m->_34 = 1.0f;
    m->_43 = -zn * zf / (zf - zn);
}

static void vec_sub(float *r, const float *a, const float *b)
{
    r[0] = a[0] - b[0]; r[1] = a[1] - b[1]; r[2] = a[2] - b[2];
}
static void vec_norm(float *v)
{
    float l = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (l > 0.0f) { v[0] /= l; v[1] /= l; v[2] /= l; }
}
static void vec_cross(float *r, const float *a, const float *b)
{
    r[0] = a[1]*b[2] - a[2]*b[1];
    r[1] = a[2]*b[0] - a[0]*b[2];
    r[2] = a[0]*b[1] - a[1]*b[0];
}
static float vec_dot(const float *a, const float *b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static void mat_look_at_lh(D3DMATRIX *m, const float *eye, const float *at,
                            const float *up)
{
    float zaxis[3], xaxis[3], yaxis[3];
    vec_sub(zaxis, at, eye); vec_norm(zaxis);
    vec_cross(xaxis, up, zaxis); vec_norm(xaxis);
    vec_cross(yaxis, zaxis, xaxis);

    m->_11 = xaxis[0]; m->_12 = yaxis[0]; m->_13 = zaxis[0]; m->_14 = 0.0f;
    m->_21 = xaxis[1]; m->_22 = yaxis[1]; m->_23 = zaxis[1]; m->_24 = 0.0f;
    m->_31 = xaxis[2]; m->_32 = yaxis[2]; m->_33 = zaxis[2]; m->_34 = 0.0f;
    m->_41 = -vec_dot(xaxis, eye);
    m->_42 = -vec_dot(yaxis, eye);
    m->_43 = -vec_dot(zaxis, eye);
    m->_44 = 1.0f;
}

static void mat_rotation_z(D3DMATRIX *m, float angle)
{
    float c = cosf(angle), s = sinf(angle);
    mat_identity(m);
    m->_11 = c;  m->_12 = s;
    m->_21 = -s; m->_22 = c;
}

//--- D3D ----------------------------------------------------------------------

static int init_device(void)
{
    D3DPRESENT_PARAMETERS pp;
    Direct3D *pD3D;
    D3DDevice *pDevice;
    D3DVIEWPORT8 vp;
    HRESULT hr;

    DbgPrint("d3d8-triangle: Direct3DCreate8\n");
    pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!pD3D) {
        DbgPrint("d3d8-triangle: Direct3DCreate8 failed\n");
        return 0;
    }

    RtlZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.Windowed = FALSE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.FullScreen_RefreshRateInHz = 60;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    Direct3D_SetPushBufferSize(512 * 1024, (512 * 1024) / 16);

    DbgPrint("d3d8-triangle: CreateDevice (640x480, Z-buffer)\n");
    hr = Direct3D_CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                               D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &pDevice);
    if (FAILED(hr)) {
        DbgPrint("d3d8-triangle: CreateDevice failed hr=0x%08x\n", (unsigned)hr);
        return 0;
    }

    RtlZeroMemory(&vp, sizeof(vp));
    vp.Width = pp.BackBufferWidth;
    vp.Height = pp.BackBufferHeight;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    D3DDevice_SetViewport(&vp);

    DbgPrint("d3d8-triangle: device ready\n");
    return 1;
}

static void init_transforms(void)
{
    D3DMATRIX proj, view, world;
    const float eye[3] = { 0.0f, 0.0f, -7.0f };
    const float at[3]  = { 0.0f, 0.0f,  0.0f };
    const float up[3]  = { 0.0f, 1.0f,  0.0f };

    mat_perspective_fov_lh(&proj, TRI_PI / 4.0f, 4.0f / 3.0f, 1.0f, 200.0f);
    D3DDevice_SetTransform(D3DTS_PROJECTION, &proj);

    mat_look_at_lh(&view, eye, at, up);
    D3DDevice_SetTransform(D3DTS_VIEW, &view);

    mat_identity(&world);
    D3DDevice_SetTransform(D3DTS_WORLD, &world);
}

int main(void)
{
    float angle = 0.0f;

    xapi_smoke_trace_line("d3d8-triangle start");

    if (!init_device()) {
        for (;;) { }
    }
    init_transforms();

    DbgPrint("d3d8-triangle: entering render loop\n");
    for (;;) {
        D3DMATRIX world;

        mat_rotation_z(&world, angle);
        D3DDevice_SetTransform(D3DTS_WORLD, &world);
        angle += 0.02f;
        if (angle > 2.0f * TRI_PI) angle -= 2.0f * TRI_PI;

        D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                        D3DCOLOR_XRGB(0, 0, 64), 1.0f, 0);

        D3DDevice_BeginScene();
        D3DDevice_SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        D3DDevice_SetRenderState(D3DRS_LIGHTING, FALSE);
        D3DDevice_SetVertexShader(TRI_FVF);
        D3DDevice_DrawVerticesUP(D3DPT_TRIANGLELIST, 3, g_triangle, sizeof(CustomVertex));
        D3DDevice_EndScene();

        D3DDevice_Swap(0);
    }

    return 0;
}
