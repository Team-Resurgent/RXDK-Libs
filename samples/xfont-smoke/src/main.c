//------------------------------------------------------------------------------
// XFONT bitmap-font text rendering -- RXDK libxfont smoke test.
//
// Opens the embedded default font (XFONT_OpenDefaultFont -- zero external
// assets, exercises the full memory-loading engine) and renders a scrolling
// demoscene-style message: each character is drawn individually into a small
// offscreen buffer via XFONT_TextOutToMemory, then stretch-blitted onto the
// back buffer at a size that pulses with a per-character sine wave (bitmap
// glyphs have no native scale API, so this is done by hand -- render small,
// blit big). The same sine phase also drives a vertical bounce, and the text
// color cycles through the rainbow via XFONT_SetTextColor.
//------------------------------------------------------------------------------

// common.h must come first: it pulls the xapi/xtl/NT environment (defining
// NT_INCLUDED + the xboxkrnl base types) so the later windows/d3d8 headers use
// our types and skip zig's MinGW <winnt.h>.
#include "common.h"   // xapi boot/trace helpers (shared with the xapi samples)
#include <stdio.h>
#include <math.h>
#include <guiddef.h>  // GUID/REFGUID -- d3d8.h's resource interfaces reference them
#include <d3d8.h>
#include <xfont.h>

#define SCREEN_W 640
#define SCREEN_H 480

// Offscreen per-character render target for XFONT_TextOutToMemory. Sized with
// headroom above the default font's ~24px cell height for bearing/descender
// overhang. Same D3DFMT_X8R8G8B8 layout as the back buffer, so the manual
// stretch-blit below is a plain DWORD copy with no per-pixel format conversion.
#define GLYPH_BUF_W 56
#define GLYPH_BUF_H 56
static DWORD g_glyphBuf[GLYPH_BUF_W * GLYPH_BUF_H];

static const WCHAR g_message[] =
    L"RXDK XFONT DEMO -- HELLO XBOX HOMEBREW -- BITMAP FONTS, SINE BOUNCE, "
    L"STRETCH-BLIT SCALING AND RAINBOW COLOR CYCLING, ALL RENDERED WITH THE "
    L"EMBEDDED DEFAULT FONT -- GREETZ TO THE SCENE -- ";

#define PI 3.14159265358979323846f

//--- "hacker terminal" background typing effect --------------------------------
// A handful of lines type themselves out one character at a time (looping,
// each on its own offset/speed so they don't retype in lockstep), drawn once
// at native size directly to the back buffer -- no stretch-blit, no color
// cycle -- so they sit as a static-looking green terminal readout behind the
// bouncing/scaling/rainbow foreground scroller drawn on top of them.

#define HACKER_LINE_COUNT  6
#define HACKER_LINE_MAXLEN 64

static const WCHAR *const g_hackerLines[HACKER_LINE_COUNT] = {
    L"root@xbox:~# bypassing kernel signature check",
    L"injecting payload into xboxkrnl.exe",
    L"decrypting eeprom.......... ACCESS OK",
    L"scanning MCPX rom for vulnerabilities",
    L"mounting hidden partition.......... DONE",
    L"establishing reverse shell to 192.168.1.184",
};

static void draw_hacker_background(XFONT *pFont, D3DSurface *pBackBuffer, float t)
{
    const D3DCOLOR hackerGreen = 0xFF33FF33u;
    int line;

    XFONT_SetTextColor(pFont, hackerGreen);

    for (line = 0; line < HACKER_LINE_COUNT; line++) {
        const WCHAR *phrase = g_hackerLines[line];
        WCHAR buf[HACKER_LINE_MAXLEN + 1];
        int phraseLen = 0;
        int typedLen;
        float lineSpeed = 12.0f;                 // chars typed per second-equivalent
        float lineOffset = (float)line * 37.0f;   // desyncs each line's cycle
        float cyclePos;
        int y = 300 + line * 22;
        int i;

        while (phrase[phraseLen] && phraseLen < HACKER_LINE_MAXLEN) {
            phraseLen++;
        }

        // Type out, hold fully typed for a while, then restart from empty.
        cyclePos = fmodf(t * lineSpeed + lineOffset, (float)(phraseLen + 25));
        typedLen = (int)cyclePos;
        if (typedLen > phraseLen) typedLen = phraseLen;
        if (typedLen < 0) typedLen = 0;

        for (i = 0; i < typedLen; i++) {
            buf[i] = phrase[i];
        }
        // Blinking cursor while still typing; blank once the line is complete.
        buf[typedLen] = (typedLen < phraseLen && fmodf(t, 0.6f) < 0.3f) ? L'_' : L' ';
        buf[typedLen + 1] = 0;

        XFONT_TextOut(pFont, pBackBuffer, buf, (unsigned)-1, 20, y);
    }
}

//--- D3D ----------------------------------------------------------------------

static D3DDevice *g_pDevice;

static int init_device(void)
{
    D3DPRESENT_PARAMETERS pp;
    Direct3D *pD3D;
    D3DVIEWPORT8 vp;
    HRESULT hr;

    DbgPrint("xfont-smoke: Direct3DCreate8\n");
    pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!pD3D) {
        DbgPrint("xfont-smoke: Direct3DCreate8 failed\n");
        return 0;
    }

    RtlZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = SCREEN_W;
    pp.BackBufferHeight = SCREEN_H;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.Windowed = FALSE;
    pp.EnableAutoDepthStencil = FALSE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.FullScreen_RefreshRateInHz = 60;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    Direct3D_SetPushBufferSize(512 * 1024, (512 * 1024) / 16);

    DbgPrint("xfont-smoke: CreateDevice (640x480)\n");
    hr = Direct3D_CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                               D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_pDevice);
    if (FAILED(hr)) {
        DbgPrint("xfont-smoke: CreateDevice failed hr=0x%08x\n", (unsigned)hr);
        return 0;
    }

    RtlZeroMemory(&vp, sizeof(vp));
    vp.Width = pp.BackBufferWidth;
    vp.Height = pp.BackBufferHeight;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    D3DDevice_SetViewport(&vp);

    DbgPrint("xfont-smoke: device ready\n");
    return 1;
}

//--- HSV -> D3DCOLOR (rainbow cycling) -----------------------------------------

static D3DCOLOR hue_to_color(float hue)
{
    float h6 = hue * 6.0f;
    int   i  = (int)h6;
    float f  = h6 - (float)i;
    float q  = 1.0f - f;
    BYTE  r, g, b;

    switch (i % 6) {
    default:
    case 0: r = 255;              g = (BYTE)(f * 255.0f); b = 0;                break;
    case 1: r = (BYTE)(q * 255.0f); g = 255;              b = 0;                break;
    case 2: r = 0;                g = 255;              b = (BYTE)(f * 255.0f); break;
    case 3: r = 0;                g = (BYTE)(q * 255.0f); b = 255;              break;
    case 4: r = (BYTE)(f * 255.0f); g = 0;                b = 255;              break;
    case 5: r = 255;              g = 0;                b = (BYTE)(q * 255.0f); break;
    }

    return 0xFF000000u | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
}

//--- Stretch-blit one rendered glyph onto the back buffer, nearest-neighbor ---

static void blit_glyph_stretched(D3DSurface *pBackBuffer, int srcW, int srcH,
                                  int dstX, int dstY, int dstW, int dstH)
{
    D3DLOCKED_RECT lock;
    DWORD *pDst;
    int pitchPixels;
    int x, y;

    if (dstW <= 0 || dstH <= 0 || srcW <= 0 || srcH <= 0) {
        return;
    }

    D3DSurface_LockRect(pBackBuffer, &lock, NULL, 0);
    pDst = (DWORD *)lock.pBits;
    pitchPixels = lock.Pitch / sizeof(DWORD);

    for (y = 0; y < dstH; y++) {
        int dy = dstY + y;
        int sy;

        if (dy < 0 || dy >= SCREEN_H) {
            continue;
        }

        sy = (y * srcH) / dstH;
        if (sy >= srcH) sy = srcH - 1;

        for (x = 0; x < dstW; x++) {
            int dx = dstX + x;
            int sx;
            DWORD pixel;

            if (dx < 0 || dx >= SCREEN_W) {
                continue;
            }

            sx = (x * srcW) / dstW;
            if (sx >= srcW) sx = srcW - 1;

            // The offscreen buffer is cleared to 0 before each glyph render and
            // XFONT paints with a transparent background by default, so any
            // pixel still 0 here was never touched by the glyph -- skip it so
            // the scrolling background shows through instead of a black box.
            pixel = g_glyphBuf[sy * GLYPH_BUF_W + sx];
            if (pixel != 0) {
                pDst[dy * pitchPixels + dx] = pixel;
            }
        }
    }

    D3DSurface_UnlockRect(pBackBuffer);
}

//--- Render loop ---------------------------------------------------------------

static void render_frame(XFONT *pFont, float t)
{
    D3DSurface *pBackBuffer;
    unsigned cellHeight, descent;
    int msgLen = (int)(sizeof(g_message) / sizeof(WCHAR)) - 1;
    unsigned totalWidth, ich;
    float scrollSpeed = 90.0f;   // pixels/sec-equivalent (frame-counted, see t)
    float startX;
    int copy;

    XFONT_GetFontMetrics(pFont, &cellHeight, &descent);
    XFONT_GetTextExtent(pFont, g_message, msgLen, &totalWidth);

    D3DDevice_GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

    D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    D3DDevice_BeginScene();

    // Background layer first: the typing "hacker terminal" readout, drawn at
    // native size directly to the surface. The colorful scroller below is
    // drawn on top of it every frame.
    draw_hacker_background(pFont, pBackBuffer, t);

    // Ticker-tape wraparound: draw two copies of the message back-to-back so
    // the screen never shows a gap while one copy scrolls off and the next
    // scrolls on. The trailing "--" spacing in g_message keeps the seam clean.
    //
    // The repeat period must cover the FULL travel distance -- from fully
    // off-screen right (x = SCREEN_W) to fully off-screen left (x =
    // -totalWidth) -- not just the message's own width. Using only
    // (totalWidth + gap) as the period was too short: it wrapped a copy back
    // to the right edge while its trailing end was still on screen, so the
    // message never appeared to fully scroll off before repeating.
    {
    const float period = (float)SCREEN_W + (float)totalWidth + 40.0f;
    for (copy = 0; copy < 2; copy++) {
        float baseX = SCREEN_W - fmodf(t * scrollSpeed, period) + copy * period;
        float x = baseX;

        if (x > (float)SCREEN_W || x + (float)totalWidth < 0.0f) {
            continue;
        }

        for (ich = 0; ich < (unsigned)msgLen; ich++) {
            WCHAR wch = g_message[ich];
            unsigned advance;
            float phase = t * 4.0f + (float)ich * 0.5f;
            float bounce = sinf(phase) * 10.0f;
            float scale = 1.0f + 0.45f * sinf(phase * 0.7f);
            float hue = fmodf(t * 0.15f + (float)ich * 0.04f, 1.0f);
            int dstW, dstH, dstX, dstY;

            XFONT_GetTextExtent(pFont, &wch, 1, &advance);

            if (scale < 0.5f) scale = 0.5f;

            dstW = (int)((float)GLYPH_BUF_W * scale * 0.6f);
            dstH = (int)((float)GLYPH_BUF_H * scale * 0.6f);
            dstX = (int)x;
            dstY = (int)(40.0f + bounce);

            if (dstX + dstW >= 0 && dstX < SCREEN_W && wch != L' ') {
                RtlZeroMemory(g_glyphBuf, sizeof(g_glyphBuf));
                XFONT_SetTextColor(pFont, hue_to_color(hue));
                XFONT_TextOutToMemory(pFont, g_glyphBuf, GLYPH_BUF_W * sizeof(DWORD),
                                      GLYPH_BUF_W, GLYPH_BUF_H, D3DFMT_X8R8G8B8,
                                      &wch, 1, 4, 4);
                blit_glyph_stretched(pBackBuffer, GLYPH_BUF_W, GLYPH_BUF_H, dstX, dstY, dstW, dstH);
            }

            x += (float)advance;
        }
    }
    }

    D3DDevice_EndScene();
    D3DDevice_Swap(0);
}

int main(void)
{
    XFONT *pFont;
    HRESULT hr;
    float t = 0.0f;

    xapi_smoke_trace_line("xfont-smoke start");

    if (!init_device()) {
        for (;;) { }
    }

    DbgPrint("xfont-smoke: XFONT_OpenDefaultFont\n");
    hr = XFONT_OpenDefaultFont(&pFont);
    if (FAILED(hr)) {
        DbgPrint("xfont-smoke: XFONT_OpenDefaultFont failed hr=0x%08x\n", (unsigned)hr);
        for (;;) { }
    }

    xapi_smoke_trace_line("xfont-smoke: font ready, entering render loop");

    for (;;) {
        render_frame(pFont, t);
        t += 0.033f;
    }

    XFONT_Release(pFont);
    return 0;
}
