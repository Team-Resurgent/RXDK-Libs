//------------------------------------------------------------------------------
// Direct3D 8 texture grid -- RXDK libd3dx8 sample.
//
// Loads each image in the title's media\ folder (bmp/dds/dib/jpg/png/tga) with
// D3DXCreateTextureFromFile (libd3dx8 -> the vendored jpeg/png/zlib codecs) and
// draws them on a 3x2 grid of pre-transformed textured quads, each quad drifting
// on a small sine/cosine orbit. Exercises libd3dx8's texture pipeline end to end.
//
// The media files are deployed alongside the XBE (D:\media\*) -- build-iso.ps1
// xbcp's samples\d3d8-textures\media to xe:\DEVKIT\xbetest\d3d8-textures\media.
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
// libd3dx8's <d3dx8tex.h> pulls the full d3dx8.h umbrella, whose COM vtables use
// the STDMETHOD* calling-convention macros. A full XDK supplies them via <xtl.h>;
// this title includes the lighter <d3d8.h> directly (common.h -> xapi.h already
// sets NT_INCLUDED + the base Win32 types), so it defines just the COM-macro /
// handle / FourCC prelude the d3dx8 headers expect before the include -- the same
// set the libd3dx8 build provides through site/bridge_d3dx8.h.
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
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);   // dxfile.h (pulled by d3dx8mesh.h) carries one
#endif
#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) \
    ((DWORD)(BYTE)(a) | ((DWORD)(BYTE)(b) << 8) | \
     ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24))
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
#include <d3dx8tex.h>

#define TEX_PI   3.14159265358979323846f
#define SCREEN_W 640.0f
#define SCREEN_H 480.0f
#define GRID_COLS 3
#define GRID_ROWS 2
#define NUM_TEX   (GRID_COLS * GRID_ROWS)
#define QUAD_HALF 80.0f   // half-extent of each textured quad, in pixels

// One texture per media file. D3DXCreateTextureFromFile picks the codec by
// content, so the mix of bmp/dds/dib/jpg/png/tga all load through the same call.
static const char *const g_media[NUM_TEX] = {
    "D:\\media\\test.bmp",
    "D:\\media\\test.dds",
    "D:\\media\\test.dib",
    "D:\\media\\test.jpg",
    "D:\\media\\test.png",
    "D:\\media\\test.tga",
};
static D3DTexture *g_tex[NUM_TEX];
static D3DDevice  *g_pDevice;

// Pre-transformed (screen-space) textured vertex: XYZRHW + one UV set.
typedef struct {
    float x, y, z, rhw;
    float u, v;
} TexVertex;
#define TEX_FVF (D3DFVF_XYZRHW | D3DFVF_TEX1)

static int init_device(void)
{
    D3DPRESENT_PARAMETERS pp;
    Direct3D *pD3D;
    D3DVIEWPORT8 vp;
    HRESULT hr;

    DbgPrint("d3d8-textures: Direct3DCreate8\n");
    pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!pD3D) {
        DbgPrint("d3d8-textures: Direct3DCreate8 failed\n");
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

    DbgPrint("d3d8-textures: CreateDevice (640x480)\n");
    hr = Direct3D_CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                               D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_pDevice);
    if (FAILED(hr)) {
        DbgPrint("d3d8-textures: CreateDevice failed hr=0x%08x\n", (unsigned)hr);
        return 0;
    }

    RtlZeroMemory(&vp, sizeof(vp));
    vp.Width = pp.BackBufferWidth;
    vp.Height = pp.BackBufferHeight;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    D3DDevice_SetViewport(&vp);

    DbgPrint("d3d8-textures: device ready\n");
    return 1;
}

// Load every media image into a texture via libd3dx8. A failed load leaves a
// NULL slot (drawn as an empty cell) so one bad/unsupported file can't hang the
// whole grid.
static void load_textures(void)
{
    int i;
    for (i = 0; i < NUM_TEX; i++) {
        HRESULT hr = D3DXCreateTextureFromFileA(g_pDevice, g_media[i], &g_tex[i]);
        if (FAILED(hr) || !g_tex[i]) {
            g_tex[i] = NULL;
            DbgPrint("d3d8-textures: load FAILED %s (hr=0x%08x)\n",
                     g_media[i], (unsigned)hr);
        } else {
            DbgPrint("d3d8-textures: loaded %s\n", g_media[i]);
        }
    }
}

// Draw one textured quad centred at (cx,cy), as a two-triangle fan.
static void draw_quad(D3DTexture *tex, float cx, float cy)
{
    TexVertex v[4];
    const float h = QUAD_HALF;

    v[0].x = cx - h; v[0].y = cy - h; v[0].z = 0.0f; v[0].rhw = 1.0f; v[0].u = 0.0f; v[0].v = 0.0f; // TL
    v[1].x = cx + h; v[1].y = cy - h; v[1].z = 0.0f; v[1].rhw = 1.0f; v[1].u = 1.0f; v[1].v = 0.0f; // TR
    v[2].x = cx + h; v[2].y = cy + h; v[2].z = 0.0f; v[2].rhw = 1.0f; v[2].u = 1.0f; v[2].v = 1.0f; // BR
    v[3].x = cx - h; v[3].y = cy + h; v[3].z = 0.0f; v[3].rhw = 1.0f; v[3].u = 0.0f; v[3].v = 1.0f; // BL

    D3DDevice_SetTexture(0, (D3DBaseTexture *)tex);
    D3DDevice_DrawVerticesUP(D3DPT_TRIANGLEFAN, 4, v, sizeof(TexVertex));
}

int main(void)
{
    float angle = 0.0f;
    const float cellW = SCREEN_W / GRID_COLS;
    const float cellH = SCREEN_H / GRID_ROWS;

    xapi_smoke_trace_line("d3d8-textures start");

    if (!init_device()) {
        for (;;) { }
    }
    load_textures();

    // Texture stage 0: sample the texture straight through (no lighting, no Z --
    // these are 2D screen-space quads). Linear min/mag for a clean scale.
    D3DDevice_SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    D3DDevice_SetRenderState(D3DRS_LIGHTING, FALSE);
    D3DDevice_SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D3DDevice_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    D3DDevice_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    D3DDevice_SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DDevice_SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);

    DbgPrint("d3d8-textures: entering render loop\n");
    for (;;) {
        int i;

        D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                        D3DCOLOR_XRGB(0, 0, 64), 1.0f, 0);

        D3DDevice_BeginScene();
        D3DDevice_SetVertexShader(TEX_FVF);

        for (i = 0; i < NUM_TEX; i++) {
            int col = i % GRID_COLS;
            int row = i / GRID_COLS;
            // Cell centre + a small per-tile orbit so every item drifts slightly.
            float cx = (col + 0.5f) * cellW + sinf(angle + (float)i * 0.9f) * 12.0f;
            float cy = (row + 0.5f) * cellH + cosf(angle + (float)i * 1.3f) * 12.0f;
            if (g_tex[i])
                draw_quad(g_tex[i], cx, cy);
        }

        D3DDevice_EndScene();
        D3DDevice_Swap(0);

        angle += 0.03f;
        if (angle > 2.0f * TEX_PI) angle -= 2.0f * TEX_PI;
    }

    return 0;
}
