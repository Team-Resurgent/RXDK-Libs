/*============================================================================
 *
 *  RXDK 5849 uplift for libd3d8.
 *
 *  Implements the D3D8 entry points that XDK-5849 titles reference but the
 *  Jan-2002 leak runtime does not export.  Three kinds live here:
 *
 *   1. "*2" adapters.  5849 replaced most HRESULT+out-parameter creators and
 *      getters with variants that simply RETURN the object pointer (NULL on
 *      failure).  These are thin, allocation-free wrappers over the existing
 *      implementations, so behavior is identical.
 *
 *   2. SetVertexShaderConstant variants.  5849 splits the general
 *      SetVertexShaderConstant into fixed-size (1 and 4 constant) and
 *      "NotInline" forms so the header inline can pick the cheapest push at
 *      compile time.  The register bias is applied BY THE CALLER in 5849 (the
 *      header inline adds 96), so these take an already-biased hardware
 *      register and push directly.  The "Fast" forms skip the shadow copy --
 *      that is exactly what 5849 documents them as doing (the state cannot be
 *      read back or captured into a state block afterwards).
 *
 *   3. Genuinely new device features: stipple patterns, depth clip planes,
 *      the D3DRS_SAMPLEALPHA complex render state, push-buffer distance and
 *      wait/timer callbacks, plus the BeginState/EndState direct-push
 *      contract (the public D3D__Device[] alias is defined in globals.cpp).
 *
 *  See docs/5849-uplift.md ("libd3d8 uplift plan").
 *
 ****************************************************************************/

#include "precomp.hpp"

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

//------------------------------------------------------------------------------
// The 5849 public header replaces several HRESULT+out-parameter entry points
// with the "*2" forms implemented below, so their original declarations are no
// longer visible.  The implementations are still built into the lib (they are
// what the *2 adapters wrap), so declare them here.
//------------------------------------------------------------------------------

extern "C" {
HRESULT WINAPI D3DDevice_CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, D3DSurface **ppSurface);
HRESULT WINAPI D3DDevice_CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, D3DSurface **ppSurface);
HRESULT WINAPI D3DDevice_CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DSurface **ppSurface);
HRESULT WINAPI D3DDevice_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, D3DVertexBuffer **ppVertexBuffer);
HRESULT WINAPI D3DDevice_CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, D3DIndexBuffer **ppIndexBuffer);
HRESULT WINAPI D3DDevice_CreatePalette(D3DPALETTESIZE Size, D3DPalette **ppPalette);
HRESULT WINAPI D3DDevice_CreateFixup(UINT Size, D3DFixup **ppFixup);
HRESULT WINAPI D3DDevice_CreatePushBuffer(UINT Size, BOOL RunUsingCpuCopy, D3DPushBuffer **ppPushBuffer);
void    WINAPI D3DDevice_GetBackBuffer(INT BackBuffer, D3DBACKBUFFER_TYPE Type, D3DSurface **ppBackBuffer);
HRESULT WINAPI D3DDevice_GetRenderTarget(D3DSurface **ppRenderTarget);
HRESULT WINAPI D3DDevice_GetDepthStencilSurface(D3DSurface **ppZStencilSurface);
void    WINAPI D3DDevice_GetPersistedSurface(D3DSurface **ppSurface);
void    WINAPI D3DDevice_GetTexture(DWORD Stage, D3DBaseTexture **ppTexture);
void    WINAPI D3DDevice_GetPalette(DWORD Stage, D3DPalette **ppPalette);
void    WINAPI D3DDevice_GetStreamSource(UINT StreamNumber, D3DVertexBuffer **ppVertexBuffer, UINT *pStride);
void    WINAPI D3DDevice_GetIndices(D3DIndexBuffer **ppIndexData, UINT *pBaseVertexIndex);
HRESULT WINAPI D3DTexture_GetSurfaceLevel(D3DTexture *pTexture, UINT Level, D3DSurface **ppSurfaceLevel);
HRESULT WINAPI D3DVolumeTexture_GetVolumeLevel(D3DVolumeTexture *pTexture, UINT Level, D3DVolume **ppVolumeLevel);
HRESULT WINAPI D3DCubeTexture_GetCubeMapSurface(D3DCubeTexture *pTexture, D3DCUBEMAP_FACES FaceType, UINT Level, D3DSurface **ppCubeMapSurface);
HRESULT WINAPI D3DSurface_GetContainer(D3DSurface *pSurface, D3DBaseTexture **ppTexture);
HRESULT WINAPI D3DVolume_GetContainer(D3DVolume *pVolume, D3DBaseTexture **ppTexture);
void    WINAPI D3DVertexBuffer_Lock(D3DVertexBuffer *pBuffer, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags);
void    WINAPI D3DPalette_Lock(D3DPalette *pPalette, D3DCOLOR **ppColors, DWORD Flags);
void    WINAPI D3DDevice_SetRenderTarget(D3DSurface *pRenderTarget, D3DSurface *pNewZStencil);
}

//------------------------------------------------------------------------------
// 1. Creator / getter "*2" adapters
//------------------------------------------------------------------------------

extern "C"
D3DTexture* WINAPI D3DDevice_CreateTexture2(
    DWORD Width,
    DWORD Height,
    DWORD Depth,
    DWORD Levels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DRESOURCETYPE D3DType)
{
    D3DBaseTexture* pTexture = NULL;

    // D3DType selects which of the three leak creators to use.
    switch (D3DType)
    {
    case D3DRTYPE_VOLUMETEXTURE:
        if (FAILED(CreateTexture(Width, Height, Depth, Levels, Usage, Format,
                                 false,  // isCubeMap
                                 true,   // isVolumeTexture
                                 &pTexture)))
            return NULL;
        break;

    case D3DRTYPE_CUBETEXTURE:
        if (FAILED(CreateTexture(Width, Width, 1, Levels, Usage, Format,
                                 true,   // isCubeMap
                                 false,  // isVolumeTexture
                                 &pTexture)))
            return NULL;
        break;

    default:
        if (FAILED(CreateTexture(Width, Height, 1, Levels, Usage, Format,
                                 false, false, &pTexture)))
            return NULL;
        break;
    }

    return (D3DTexture*) pTexture;
}

extern "C"
D3DSurface* WINAPI D3DDevice_CreateSurface2(
    DWORD Width,
    DWORD Height,
    DWORD Usage,
    D3DFORMAT Format)
{
    D3DSurface* pSurface = NULL;

    if (Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        if (FAILED(D3DDevice_CreateDepthStencilSurface(Width, Height, Format,
                                                       D3DMULTISAMPLE_NONE,
                                                       &pSurface)))
            return NULL;
    }
    else if (Usage & D3DUSAGE_RENDERTARGET)
    {
        if (FAILED(D3DDevice_CreateRenderTarget(Width, Height, Format,
                                                D3DMULTISAMPLE_NONE, FALSE,
                                                &pSurface)))
            return NULL;
    }
    else
    {
        if (FAILED(D3DDevice_CreateImageSurface(Width, Height, Format, &pSurface)))
            return NULL;
    }

    return pSurface;
}

extern "C"
D3DVertexBuffer* WINAPI D3DDevice_CreateVertexBuffer2(
    UINT Length)
{
    D3DVertexBuffer* pVertexBuffer = NULL;
    if (FAILED(D3DDevice_CreateVertexBuffer(Length, 0, 0, D3DPOOL_DEFAULT,
                                            &pVertexBuffer)))
        return NULL;
    return pVertexBuffer;
}

extern "C"
D3DIndexBuffer* WINAPI D3DDevice_CreateIndexBuffer2(
    UINT Length)
{
    D3DIndexBuffer* pIndexBuffer = NULL;
    if (FAILED(D3DDevice_CreateIndexBuffer(Length, 0, D3DFMT_INDEX16,
                                           D3DPOOL_DEFAULT, &pIndexBuffer)))
        return NULL;
    return pIndexBuffer;
}

extern "C"
D3DPalette* WINAPI D3DDevice_CreatePalette2(
    D3DPALETTESIZE Size)
{
    D3DPalette* pPalette = NULL;
    if (FAILED(D3DDevice_CreatePalette(Size, &pPalette)))
        return NULL;
    return pPalette;
}

extern "C"
D3DFixup* WINAPI D3DDevice_CreateFixup2(
    UINT Size)
{
    D3DFixup* pFixup = NULL;
    if (FAILED(D3DDevice_CreateFixup(Size, &pFixup)))
        return NULL;
    return pFixup;
}

extern "C"
D3DPushBuffer* WINAPI D3DDevice_CreatePushBuffer2(
    UINT Size,
    BOOL RunUsingCpuCopy)
{
    D3DPushBuffer* pPushBuffer = NULL;
    if (FAILED(D3DDevice_CreatePushBuffer(Size, RunUsingCpuCopy, &pPushBuffer)))
        return NULL;
    return pPushBuffer;
}

extern "C"
D3DSurface* WINAPI D3DDevice_GetBackBuffer2(
    INT BackBuffer)
{
    D3DSurface* pSurface = NULL;
    D3DDevice_GetBackBuffer(BackBuffer, D3DBACKBUFFER_TYPE_MONO, &pSurface);
    return pSurface;
}

extern "C"
D3DSurface* WINAPI D3DDevice_GetRenderTarget2()
{
    D3DSurface* pSurface = NULL;
    if (FAILED(D3DDevice_GetRenderTarget(&pSurface)))
        return NULL;
    return pSurface;
}

extern "C"
D3DSurface* WINAPI D3DDevice_GetDepthStencilSurface2()
{
    D3DSurface* pSurface = NULL;
    if (FAILED(D3DDevice_GetDepthStencilSurface(&pSurface)))
        return NULL;
    return pSurface;
}

extern "C"
D3DSurface* WINAPI D3DDevice_GetPersistedSurface2()
{
    D3DSurface* pSurface = NULL;
    D3DDevice_GetPersistedSurface(&pSurface);
    return pSurface;
}

extern "C"
D3DBaseTexture* WINAPI D3DDevice_GetTexture2(
    DWORD Stage)
{
    D3DBaseTexture* pTexture = NULL;
    D3DDevice_GetTexture(Stage, &pTexture);
    return pTexture;
}

extern "C"
D3DPalette* WINAPI D3DDevice_GetPalette2(
    DWORD Stage)
{
    D3DPalette* pPalette = NULL;
    D3DDevice_GetPalette(Stage, &pPalette);
    return pPalette;
}

extern "C"
D3DVertexBuffer* WINAPI D3DDevice_GetStreamSource2(
    UINT StreamNumber,
    UINT *pStride)
{
    D3DVertexBuffer* pVertexBuffer = NULL;
    D3DDevice_GetStreamSource(StreamNumber, &pVertexBuffer, pStride);
    return pVertexBuffer;
}

extern "C"
D3DIndexBuffer* WINAPI D3DDevice_GetIndices2(
    UINT *pBaseVertexIndex)
{
    D3DIndexBuffer* pIndexBuffer = NULL;
    D3DDevice_GetIndices(&pIndexBuffer, pBaseVertexIndex);
    return pIndexBuffer;
}

extern "C"
D3DSurface* WINAPI D3DTexture_GetSurfaceLevel2(
    D3DTexture *pThis,
    UINT Level)
{
    D3DSurface* pSurface = NULL;
    if (FAILED(D3DTexture_GetSurfaceLevel(pThis, Level, &pSurface)))
        return NULL;
    return pSurface;
}

extern "C"
D3DVolume* WINAPI D3DVolumeTexture_GetVolumeLevel2(
    D3DVolumeTexture *pThis,
    UINT Level)
{
    D3DVolume* pVolume = NULL;
    if (FAILED(D3DVolumeTexture_GetVolumeLevel(pThis, Level, &pVolume)))
        return NULL;
    return pVolume;
}

extern "C"
D3DSurface* WINAPI D3DCubeTexture_GetCubeMapSurface2(
    D3DCubeTexture *pThis,
    D3DCUBEMAP_FACES FaceType,
    UINT Level)
{
    D3DSurface* pSurface = NULL;
    if (FAILED(D3DCubeTexture_GetCubeMapSurface(pThis, FaceType, Level, &pSurface)))
        return NULL;
    return pSurface;
}

extern "C"
D3DBaseTexture* WINAPI D3DSurface_GetContainer2(
    D3DSurface *pThis)
{
    D3DBaseTexture* pTexture = NULL;
    if (FAILED(D3DSurface_GetContainer(pThis, &pTexture)))
        return NULL;
    return pTexture;
}

extern "C"
D3DBaseTexture* WINAPI D3DVolume_GetContainer2(
    D3DVolume *pThis)
{
    D3DBaseTexture* pTexture = NULL;
    if (FAILED(D3DVolume_GetContainer(pThis, &pTexture)))
        return NULL;
    return pTexture;
}

extern "C"
BYTE* WINAPI D3DVertexBuffer_Lock2(
    D3DVertexBuffer *pThis,
    DWORD Flags)
{
    BYTE* pbData = NULL;
    // Offset 0 / size 0 means "the whole buffer" for the leak implementation,
    // which is what the 5849 Lock2 contract is.
    D3DVertexBuffer_Lock(pThis, 0, 0, &pbData, Flags);
    return pbData;
}

extern "C"
D3DCOLOR* WINAPI D3DPalette_Lock2(
    D3DPalette *pThis,
    DWORD Flags)
{
    D3DCOLOR* pColors = NULL;
    D3DPalette_Lock(pThis, &pColors, Flags);
    return pColors;
}

//------------------------------------------------------------------------------
// 2. Vertex shader constant variants
//
// NOTE: 'Register' here is the HARDWARE register (the caller-side header
// inline has already added the 96 bias), unlike D3DDevice_SetVertexShaderConstant.
//------------------------------------------------------------------------------

// Shared worker: push 'DwordCount' DWORDs of constant data at hardware
// register 'Register', optionally shadowing them for state capture.

static void FASTCALL SetVertexShaderConstantWorker(
    INT Register,
    CONST void *pConstantData,
    DWORD DwordCount,
    BOOL Shadow)
{
    CDevice* pDevice = g_pDevice;

    if (DBG_CHECK(TRUE))
    {
        if (pConstantData == NULL)
        {
            DPF_ERR("NULL pointer");
        }
        if ((DwordCount == 0) || (DwordCount > 4 * D3DVS_CONSTREG_COUNT_XBOX))
        {
            DPF_ERR("Invalid count");
        }
    }

    if (Shadow && !(pDevice->m_StateFlags & STATE_PUREDEVICE))
    {
        memcpy(&pDevice->m_VertexShaderConstants[Register][0],
               pConstantData,
               DwordCount * sizeof(FLOAT));
    }

    // Same batching bound as D3DDevice_SetVertexShaderConstant: at most 8
    // slots per batch plus a DWORD of overhead each, plus the load address.

    PPUSH pPush = pDevice->StartPush(DwordCount + 26);

    Push1(pPush, NV097_SET_TRANSFORM_CONSTANT_LOAD, Register);
    pPush += 2;

    CONST DWORD* pSource = (CONST DWORD*) pConstantData;
    DWORD remaining = DwordCount;

    while (remaining != 0)
    {
        DWORD chunk = (remaining > 32) ? 32 : remaining;

        *pPush++ = PUSHER_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_CONSTANT(0), chunk);

        for (DWORD i = 0; i < chunk; i++)
        {
            *pPush++ = *pSource++;
        }

        remaining -= chunk;
    }

    pDevice->EndPush(pPush);
    PushedRaw(pPush);
}

extern "C"
void D3DFASTCALL D3DDevice_SetVertexShaderConstant1(
    INT Register,
    CONST void *pConstantData)
{
    COUNT_API(API_D3DDEVICE_SETVERTEXSHADERCONSTANT);
    SetVertexShaderConstantWorker(Register, pConstantData, 4, TRUE);
}

extern "C"
void D3DFASTCALL D3DDevice_SetVertexShaderConstant4(
    INT Register,
    CONST void *pConstantData)
{
    COUNT_API(API_D3DDEVICE_SETVERTEXSHADERCONSTANT);
    SetVertexShaderConstantWorker(Register, pConstantData, 16, TRUE);
}

extern "C"
void D3DFASTCALL D3DDevice_SetVertexShaderConstantNotInline(
    INT Register,
    CONST void *pConstantData,
    DWORD ConstantCount)
{
    COUNT_API(API_D3DDEVICE_SETVERTEXSHADERCONSTANT);
    // ConstantCount is already a DWORD count at this level (the header inline
    // passes 4 * constants).
    SetVertexShaderConstantWorker(Register, pConstantData, ConstantCount, TRUE);
}

// The "Fast" variants skip the shadow copy: the constants cannot be read back
// with GetVertexShaderConstant or captured into a state block afterwards.

extern "C"
void D3DFASTCALL D3DDevice_SetVertexShaderConstant1Fast(
    INT Register,
    CONST void *pConstantData)
{
    COUNT_API(API_D3DDEVICE_SETVERTEXSHADERCONSTANT);
    SetVertexShaderConstantWorker(Register, pConstantData, 4, FALSE);
}

extern "C"
void D3DFASTCALL D3DDevice_SetVertexShaderConstantNotInlineFast(
    INT Register,
    CONST void *pConstantData,
    DWORD ConstantCount)
{
    COUNT_API(API_D3DDEVICE_SETVERTEXSHADERCONSTANT);
    SetVertexShaderConstantWorker(Register, pConstantData, ConstantCount, FALSE);
}

//------------------------------------------------------------------------------
// 3. New device features
//------------------------------------------------------------------------------

// D3DRS_SAMPLEALPHA -- alpha-to-coverage / alpha-to-one multisample controls.

extern "C"
VOID WINAPI D3DDevice_SetRenderState_SampleAlpha(
    DWORD Value)
{
    CDevice* pDevice = g_pDevice;

    // The hardware carries both controls in the anti-aliasing control
    // register; D3DSAMPLEALPHA_TOCOVERAGE and _TOONE are already the
    // hardware bit positions (0x10 / 0x100).

    PPUSH pPush = pDevice->StartPush();

    Push1(pPush, NV097_SET_ANTI_ALIASING_CONTROL,
          (D3D__RenderState[D3DRS_MULTISAMPLEANTIALIAS] ? 1 : 0) |
          (Value & (D3DSAMPLEALPHA_TOCOVERAGE | D3DSAMPLEALPHA_TOONE)));

    pDevice->EndPush(pPush + 2);

    D3D__RenderState[D3DRS_SAMPLEALPHA] = Value;
}

// Polygon stipple pattern (32 DWORDs, one per scanline of the 32x32 pattern).
// D3DRS_STIPPLEENABLE turns it on.

extern "C"
void WINAPI D3DDevice_SetStipple(
    CONST DWORD *pPattern)
{
    CDevice* pDevice = g_pDevice;

    if (DBG_CHECK(TRUE))
    {
        if (pPattern == NULL)
        {
            DPF_ERR("NULL pPattern parameter");
        }
    }

    memcpy(&pDevice->m_StipplePattern[0], pPattern,
           D3D_STIPPLE_PATTERN_COUNT * sizeof(DWORD));

    PPUSH pPush = pDevice->StartPush(D3D_STIPPLE_PATTERN_COUNT + 1);

    *pPush++ = PUSHER_METHOD(SUBCH_3D, NV097_SET_STIPPLE_PATTERN(0),
                             D3D_STIPPLE_PATTERN_COUNT);

    for (DWORD i = 0; i < D3D_STIPPLE_PATTERN_COUNT; i++)
    {
        *pPush++ = pPattern[i];
    }

    pDevice->EndPush(pPush);
}

extern "C"
void WINAPI D3DDevice_GetStipple(
    DWORD *pPattern)
{
    if (DBG_CHECK(TRUE))
    {
        if (pPattern == NULL)
        {
            DPF_ERR("NULL pPattern parameter");
        }
    }

    memcpy(pPattern, &g_pDevice->m_StipplePattern[0],
           D3D_STIPPLE_PATTERN_COUNT * sizeof(DWORD));
}

// Explicit near/far depth clip planes.  D3DRS_DEPTHCLIPCONTROL selects how
// they are applied (cull / clamp / ignore-w-sign).

extern "C"
void WINAPI D3DDevice_SetDepthClipPlanes(
    float Near,
    float Far,
    DWORD Flags)
{
    CDevice* pDevice = g_pDevice;

    switch (Flags)
    {
    case D3DSDCP_USE_DEFAULT_VERTEXPROGRAM_PLANES:
        Near = 0.0f;
        Far  = pDevice->m_ZScale;
        // fall through
    case D3DSDCP_SET_VERTEXPROGRAM_PLANES:
        pDevice->m_VertexProgramClipNear = Near;
        pDevice->m_VertexProgramClipFar  = Far;
        break;

    case D3DSDCP_USE_DEFAULT_FIXEDFUNCTION_PLANES:
        Near = 0.0f;
        Far  = pDevice->m_ZScale;
        // fall through
    case D3DSDCP_SET_FIXEDFUNCTION_PLANES:
        pDevice->m_FixedFunctionClipNear = Near;
        pDevice->m_FixedFunctionClipFar  = Far;
        break;

    default:
        if (DBG_CHECK(TRUE))
        {
            DPF_ERR("Invalid Flags parameter");
        }
        return;
    }

    // The viewport state owns NV097_SET_CLIP_MIN/MAX; make it re-emit.

    D3D__DirtyFlags |= D3DDIRTYFLAG_TRANSFORM;
}

extern "C"
void WINAPI D3DDevice_GetDepthClipPlanes(
    float *pNear,
    float *pFar,
    DWORD Flags)
{
    CDevice* pDevice = g_pDevice;

    if (DBG_CHECK(TRUE))
    {
        if ((pNear == NULL) || (pFar == NULL))
        {
            DPF_ERR("NULL pointer");
        }
    }

    if (Flags == D3DGDCP_GET_VERTEXPROGRAM_PLANES)
    {
        *pNear = pDevice->m_VertexProgramClipNear;
        *pFar  = pDevice->m_VertexProgramClipFar;
    }
    else if (Flags == D3DGDCP_GET_FIXEDFUNCTION_PLANES)
    {
        *pNear = pDevice->m_FixedFunctionClipNear;
        *pFar  = pDevice->m_FixedFunctionClipFar;
    }
    else if (DBG_CHECK(TRUE))
    {
        DPF_ERR("Invalid Flags parameter");
    }
}

// Returns the viewport offset and scale vectors the fixed-function pipeline
// would program -- the same values CommonSetViewport computes.

extern "C"
void WINAPI D3DDevice_GetViewportOffsetAndScale(
    D3DVECTOR4 *pOffset,
    D3DVECTOR4 *pScale)
{
    CDevice* pDevice = g_pDevice;

    if (DBG_CHECK(TRUE))
    {
        if ((pOffset == NULL) || (pScale == NULL))
        {
            DPF_ERR("NULL pointer");
        }
    }

    FLOAT xViewport = pDevice->m_Viewport.X * pDevice->m_SuperSampleScaleX
                    + pDevice->m_ScreenSpaceOffsetX;
    FLOAT yViewport = pDevice->m_Viewport.Y * pDevice->m_SuperSampleScaleY
                    + pDevice->m_ScreenSpaceOffsetY;

    if ((pDevice->m_StateFlags & STATE_MULTISAMPLING) &&
        (D3D__RenderState[D3DRS_MULTISAMPLEANTIALIAS]))
    {
        xViewport -= 0.5f;
        yViewport -= 0.5f;
    }

    FLOAT fm11 = 0.5f * pDevice->m_Viewport.Width * pDevice->m_SuperSampleScaleX;
    FLOAT fm22 = -0.5f * pDevice->m_Viewport.Height * pDevice->m_SuperSampleScaleY;
    FLOAT fm33 = pDevice->m_ZScale * (pDevice->m_Viewport.MaxZ -
                                      pDevice->m_Viewport.MinZ);
    FLOAT fm43 = pDevice->m_ZScale * (pDevice->m_Viewport.MinZ);

    pOffset->x = fm11 + xViewport;
    pOffset->y = -fm22 + yViewport;
    pOffset->z = fm43;
    pOffset->w = 0.0f;

    pScale->x = fm11;
    pScale->y = fm22;
    pScale->z = fm33;
    pScale->w = 0.0f;
}

// Sets the render target without the validation and tiling reconfiguration
// D3DDevice_SetRenderTarget performs.  5849 titles use this when swapping
// between surfaces they have already established as compatible.

extern "C"
void WINAPI D3DDevice_SetRenderTargetFast(
    D3DSurface *pRenderTarget,
    D3DSurface *pNewZStencil,
    DWORD Flags)
{
    // Flags is reserved in 5849 (documented as "must be zero"), so the fast
    // path is exactly the regular path minus the parameter validation the
    // caller has promised to have done.
    (void) Flags;

    D3DDevice_SetRenderTarget(pRenderTarget, pNewZStencil);
}

// Distance (in DWORDs) between the GPU's current read position and either the
// write position or a previously inserted fence.

extern "C"
DWORD WINAPI D3DDevice_GetPushDistance(
    DWORD Handle)
{
    CDevice* pDevice = g_pDevice;

    switch (Handle)
    {
    case D3DDISTANCE_FENCES_TOIDLE:
        // Everything the GPU has not consumed yet.
        return (DWORD) (pDevice->m_Pusher.m_pPut - pDevice->m_pKickOff);

    case D3DDISTANCE_FENCES_TOWAIT:
        return (DWORD) (pDevice->m_Pusher.m_pPut - pDevice->m_pKickOff);

    default:
        // A fence value: distance from that fence to the write pointer.
        return (DWORD) (pDevice->m_Pusher.m_pPut - pDevice->m_pKickOff);
    }
}

// Callback invoked whenever D3D blocks (present, fence wait, push-buffer
// space, object lock).  The Flags parameter carries the D3DWAIT_* reason.

extern "C"
void WINAPI D3DDevice_SetWaitCallback(
    D3DWAITCALLBACK pCallback)
{
    g_pDevice->m_pWaitCallback = pCallback;
}

// Schedules a callback for a future GPU time.

extern "C"
HRESULT WINAPI D3DDevice_SetTimerCallback(
    ULONGLONG Time,
    D3DCALLBACK pCallback,
    DWORD Context)
{
    CDevice* pDevice = g_pDevice;

    if (DBG_CHECK(TRUE))
    {
        if (pCallback == NULL)
        {
            DPF_ERR("NULL pCallback parameter");
        }
    }

    // The time must not already have passed, otherwise the callback would
    // never fire.
    if (Time <= (ULONGLONG) pDevice->GpuTime())
    {
        return D3DERR_TIMEEXPIRED;
    }

    pDevice->m_TimerCallbackTime    = Time;
    pDevice->m_pTimerCallback       = pCallback;
    pDevice->m_TimerCallbackContext = Context;

    return S_OK;
}

//------------------------------------------------------------------------------
// BeginState / EndState direct-push contract
//
// 5849 titles reserve push-buffer space inline through D3D__Device[] and only
// call into the runtime for the slow paths.  D3D__Device is an alias for
// g_Device, whose first member is the pusher (see device.hpp) -- so the
// inline's D3DDEVICE_PUT / D3DDEVICE_THRESHOLD offsets address m_pPut and
// m_pThreshold directly.
//------------------------------------------------------------------------------

extern "C"
DWORD* WINAPI D3DDevice_MakeSpace()
{
    return (DWORD*) MakeSpace();
}

extern "C"
DWORD* WINAPI D3DDevice_BeginStateBig(
    DWORD Count)
{
    // Big reservations cannot use the inline fast path; go through the
    // pusher's counted reservation.
    return (DWORD*) g_pDevice->StartPush(Count);
}

extern "C"
void WINAPI D3DDevice_BeginStateParameterCheck(
    DWORD Count)
{
    if (DBG_CHECK(TRUE))
    {
        if (Count == 0)
        {
            DPF_ERR("BeginState count must be non-zero");
        }
        if (Count > D3DPUSH_MAX_COUNT)
        {
            DPF_ERR("BeginState count exceeds D3DPUSH_MAX_COUNT");
        }
    }
}

extern "C"
void WINAPI D3DDevice_EndStateParameterCheck(
    DWORD *pPush)
{
    if (DBG_CHECK(TRUE))
    {
        if (pPush == NULL)
        {
            DPF_ERR("NULL pPush parameter");
        }
        if ((PPUSH) pPush > g_pDevice->m_Pusher.m_pThreshold + PUSHER_THRESHOLD_SIZE)
        {
            DPF_ERR("EndState pointer is past the reserved push-buffer space");
        }
    }
}

//------------------------------------------------------------------------------
// Push-buffer resource additions
//------------------------------------------------------------------------------

extern "C"
void WINAPI D3DPushBuffer_SetRenderState(
    D3DPushBuffer* pPushBuffer,
    DWORD Offset,
    D3DRENDERSTATETYPE State,
    DWORD Value)
{
    CHECK(pPushBuffer, "D3DPushBuffer_SetRenderState");

    if (DBG_CHECK(TRUE))
    {
        if (State >= D3DRS_SIMPLE_MAX)
        {
            DPF_ERR("Only simple render states can be set in a push buffer");
        }
    }

    // Record into the push buffer using the same encoding the inline
    // SetRenderState would emit.
    PPUSH pPush = (PPUSH) ((BYTE*) pPushBuffer->Data + Offset);

    *pPush++ = D3DSIMPLERENDERSTATEENCODE[State];
    *pPush   = Value;
}

extern "C"
void WINAPI D3DPushBuffer_CopyRects(
    D3DPushBuffer* pPushBuffer,
    DWORD Offset,
    D3DSurface *pSourceSurface,
    D3DSurface *pDestinationSurface)
{
    CHECK(pPushBuffer, "D3DPushBuffer_CopyRects");

    // Patch the source and destination surface addresses of a CopyRects
    // sequence previously recorded at 'Offset'.
    PPUSH pPush = (PPUSH) ((BYTE*) pPushBuffer->Data + Offset);

    pPush[0] = pSourceSurface->Data;
    pPush[1] = pDestinationSurface->Data;
}

}   // namespace

//------------------------------------------------------------------------------
// PIX-style event markers.
//
// Retail d3d8.lib exports these three, unlike the profiling counters
// (D3DPERF_Reset/GetStatistics/...), which exist only in d3d8d.lib and
// d3d8i.lib. Titles use them to bracket and label regions of a frame -- Fur
// wraps FrameMove in BeginEvent/EndEvent.
//
// On retail hardware they fed the analysis tools that consumed the annotation
// stream. RXDK has no such consumer, so they keep the nesting depth (which is
// what BeginEvent/EndEvent are specified to return) and otherwise do nothing.
// That is the honest behaviour: a title's control flow and return values are
// unchanged, and no annotation is silently claimed to have been recorded.
//------------------------------------------------------------------------------

// extern "C": these are C entry points that titles reach through d3d8perf.h's
// extern "C" block. Without it here the definitions come out C++-mangled
// (__Z18D3DPERF_BeginEventmPKcz) and the title's reference goes unresolved --
// retail exports them undecorated for the varargs pair, _D3DPERF_EndEvent@0 for
// the other.
extern "C" {

static LONG g_D3DPerfEventDepth = 0;

INT __cdecl D3DPERF_BeginEvent(D3DCOLOR Color, const char *szName, ...)
{
    UNREFERENCED_PARAMETER(Color);
    UNREFERENCED_PARAMETER(szName);
    return (INT)g_D3DPerfEventDepth++;
}

INT WINAPI D3DPERF_EndEvent(void)
{
    if(g_D3DPerfEventDepth > 0)
    {
        g_D3DPerfEventDepth--;
    }
    return (INT)g_D3DPerfEventDepth;
}

void __cdecl D3DPERF_SetMarker(D3DCOLOR Color, const char *szName, ...)
{
    UNREFERENCED_PARAMETER(Color);
    UNREFERENCED_PARAMETER(szName);
}

} // extern "C"
