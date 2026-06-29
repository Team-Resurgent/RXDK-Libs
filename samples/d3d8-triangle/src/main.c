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

// --- D3DX8 ---------------------------------------------------------------------
// This title uses libd3dx8 for its matrix math (identity / projection / view /
// rotation). The public <d3dx8math.h> pulls the full d3dx8.h umbrella, which
// builds its COM vtables with the STDMETHOD* calling-convention macros. In a
// full XDK those come in via <xtl.h>; this sample includes the lighter <d3d8.h>
// directly (common.h -> xapi.h already sets NT_INCLUDED + the base Win32 types),
// so we supply just the COM macros the d3dx8 headers expect, then include the
// header. (The same set the libd3dx8 build provides via site/bridge_d3dx8.h.)
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE   __stdcall
#define STDMETHODVCALLTYPE  __cdecl
#define STDAPICALLTYPE      __stdcall
#define STDAPIVCALLTYPE     __cdecl
#endif
#ifndef EXTERN_C
#define EXTERN_C extern
#endif
#ifndef STDAPI
#define STDAPI       EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(t)   EXTERN_C t STDAPICALLTYPE
#endif
// HMODULE: dxfile.h (pulled by d3dx8mesh.h via the umbrella) carries one; not in
// our slimmed windef.h.
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif
#include <d3dx8math.h>

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
    D3DXMATRIX proj, view, world;
    D3DXVECTOR3 eye = { 0.0f, 0.0f, -7.0f };
    D3DXVECTOR3 at  = { 0.0f, 0.0f,  0.0f };
    D3DXVECTOR3 up  = { 0.0f, 1.0f,  0.0f };

    D3DXMatrixPerspectiveFovLH(&proj, TRI_PI / 4.0f, 4.0f / 3.0f, 1.0f, 200.0f);
    D3DDevice_SetTransform(D3DTS_PROJECTION, (D3DMATRIX *)&proj);

    D3DXMatrixLookAtLH(&view, &eye, &at, &up);
    D3DDevice_SetTransform(D3DTS_VIEW, (D3DMATRIX *)&view);

    D3DXMatrixIdentity(&world);
    D3DDevice_SetTransform(D3DTS_WORLD, (D3DMATRIX *)&world);
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
        D3DXMATRIX world;

        // Spin via libd3dx8 (D3DXMATRIX is layout-compatible with D3DMATRIX).
        D3DXMatrixRotationZ(&world, angle);
        D3DDevice_SetTransform(D3DTS_WORLD, (D3DMATRIX *)&world);
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
