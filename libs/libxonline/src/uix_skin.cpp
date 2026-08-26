/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// RXDK UIX skin (.uix / "XSK0") runtime loader + IPluginSupport, per the format
// produced by Rxdk.SkinBld (UixWriter.cs). Serves strings/layouts/images/audio by
// resource id to title extension features (e.g. the UIXKeyboard on-screen keyboard),
// which the built-in UIX features never needed.
//
// Layout: [FILE HEADER 20][SECTION TABLE N*20][section payloads...]. Each section
// payload = [OBJECT TABLE ObjectCount*8][blob]. Object BlobOffset is relative to the
// section's blob start; 0xFFFFFFFF = absent. Image/screen blobs embed an Xbox packed
// resource ("XPR0") whose D3DTexture headers we fix up to point at the pixel data.

#include <xtl.h>
#include <xobjbase.h>
#include <d3d8.h>
#include <xonline.h>
#include <uix.h>
#include "uix_skin.h"

// Process-heap alloc for bookkeeping; the skin BLOB itself goes in contiguous
// write-combined memory so the GPU can sample the embedded textures.
static void *SkAlloc(DWORD cb) { return HeapAlloc(GetProcessHeap(), 0, cb); }
static void  SkFree(void *pv)  { if (pv) HeapFree(GetProcessHeap(), 0, pv); }

#pragma pack(push, 1)
struct XSK_HEADER {
    char     Magic[4];              // "XSK0"
    WORD     RecordSize;            // 20
    WORD     SectionCount;
    char     AppName[8];            // "UIX"
    DWORD    BuiltInSectionCount;   // informational
};
struct XSK_SECTION {
    DWORD    RecordId;              // (Language<<16) | SectionId
    WORD     Kind;                 // 0=Screen 1=String 2=Image 3=Audio
    WORD     ObjectCount;
    DWORD    PayloadOffset;        // absolute from file start
    DWORD    XprOffset;            // relative to blob start, or 0xFFFFFFFF
    DWORD    PayloadSize;
};
struct XSK_OBJECT {
    DWORD    ResId;
    DWORD    BlobOffset;           // relative to blob start, or 0xFFFFFFFF
};
struct XPR_HEADER {
    DWORD    Magic;                // "XPR0" 0x30525058
    DWORD    TotalSize;
    DWORD    HeaderSize;
};
#pragma pack(pop)

#define XSK_KIND_SCREEN 0
#define XSK_KIND_STRING 1
#define XSK_KIND_IMAGE  2
#define XSK_KIND_AUDIO  3
#define XSK_ABSENT      0xFFFFFFFF
#define XSK_ICON_MARKER 0xE801
#define XPR_MAGIC       0x30525058u  // 'XPR0'
#define D3D_MAX_PHYSICAL_OFFSET 0x08000000u  // 128MB; matches libd3d8 XMETAL_MAX_PHYSICAL_OFFSET

#define XSK_MAX_XPR     16

struct UIX_SKIN {
    BYTE  *pBase;      // plain-heap copy of the whole .uix (headers/layouts/strings; CPU-read)
    DWORD  cbBase;
    DWORD  lang;
    const XSK_SECTION *pSections;
    DWORD  cSections;
    void  *vidmem[XSK_MAX_XPR];  // per-XPR contiguous write-combined pixel buffers (GPU-read)
    DWORD  cVidmem;
    char   audioNarrow[64]; // scratch for the last GetAudioName (narrowed to ANSI)
};

static const XSK_OBJECT *SectionObjects(UIX_SKIN *s, const XSK_SECTION *sec)
{
    return (const XSK_OBJECT *)(s->pBase + sec->PayloadOffset);
}
static const BYTE *SectionBlob(UIX_SKIN *s, const XSK_SECTION *sec)
{
    return s->pBase + sec->PayloadOffset + (DWORD)sec->ObjectCount * 8;
}

// Find a section by kind and (optionally) low-word section id, honoring language
// (exact match, else neutral/language 0, else first of the kind).
static const XSK_SECTION *FindSection(UIX_SKIN *s, WORD kind, DWORD wantSectionId,
                                      BOOL matchId)
{
    const XSK_SECTION *neutral = NULL;
    const XSK_SECTION *anyKind = NULL;
    for (DWORD i = 0; i < s->cSections; i++) {
        const XSK_SECTION *sec = &s->pSections[i];
        if (sec->Kind != kind)
            continue;
        if (matchId && (sec->RecordId & 0xFFFF) != (wantSectionId & 0xFFFF))
            continue;
        DWORD lang = sec->RecordId >> 16;
        if (lang == s->lang)
            return sec;                 // exact language
        if (lang == 0 && !neutral)
            neutral = sec;              // neutral fallback
        if (!anyKind)
            anyKind = sec;
    }
    return neutral ? neutral : anyKind;
}

// Resolve a resource id inside a section to its blob pointer (NULL if absent).
static const BYTE *ResolveObject(UIX_SKIN *s, const XSK_SECTION *sec, DWORD resId,
                                 DWORD *pBlobOffset)
{
    if (!sec)
        return NULL;
    const XSK_OBJECT *objs = SectionObjects(s, sec);
    for (DWORD i = 0; i < sec->ObjectCount; i++) {
        if (objs[i].ResId == resId && objs[i].BlobOffset != XSK_ABSENT) {
            if (pBlobOffset) *pBlobOffset = objs[i].BlobOffset;
            return SectionBlob(s, sec) + objs[i].BlobOffset;
        }
    }
    return NULL;
}

// Split an embedded XPR the way the retail packed-resource loader does: the pixel
// data (everything past HeaderSize) is copied into its OWN contiguous, texture-
// aligned, write-combined buffer, and every D3DTexture header's Data field is
// rebased from a vidmem-section-relative offset to that buffer's real address. The
// NV2A requires a D3DTEXTURE_ALIGNMENT (128B) texture base; pointing textures at an
// arbitrary offset inside a shared blob hard-locks the GPU.
static void FixupXpr(UIX_SKIN *s, const XSK_SECTION *sec)
{
    if (sec->XprOffset == XSK_ABSENT)
        return;
    BYTE *xpr = (BYTE *)SectionBlob(s, sec) + sec->XprOffset;
    if (xpr + sizeof(XPR_HEADER) > s->pBase + s->cbBase)
        return;
    XPR_HEADER *xh = (XPR_HEADER *)xpr;
    if (xh->Magic != XPR_MAGIC || xh->HeaderSize < 12 || xh->HeaderSize > xh->TotalSize)
        return;
    DWORD vidSize = xh->TotalSize - xh->HeaderSize;
    BYTE *diskPixels = xpr + xh->HeaderSize;
    if (vidSize == 0 || diskPixels + vidSize > s->pBase + s->cbBase)
        return;
    if (s->cVidmem >= XSK_MAX_XPR)
        return;

    // Must come from D3D's contiguous pool: it is guaranteed to sit inside the NV2A
    // texture-DMA window. A raw MmAllocateContiguousMemory can land outside it, and
    // the GPU then faults on the texture fetch (blue-screen on HW / DMA assert on xemu).
    BYTE *vid = (BYTE *)D3D_AllocContiguousMemory(vidSize, D3DTEXTURE_ALIGNMENT);
    if (!vid)
        return;
    memcpy(vid, diskPixels, vidSize);
    s->vidmem[s->cVidmem++] = vid;

    // Header array of D3DTexture (20 bytes) runs from after the 12-byte XPR header
    // up to HeaderSize, terminated by a 0xFFFFFFFF Common field.
    for (BYTE *p = xpr + 12; p + 20 <= xpr + xh->HeaderSize; p += 20) {
        DWORD common = *(DWORD *)p;
        if (common == XSK_ABSENT)
            break;
        DWORD *pData = (DWORD *)(p + 4);   // D3DResource.Data (vidmem-relative offset)
        // The runtime pushes D3DTexture::Data straight into NV_PGRAPH_TEXOFFSET, so it
        // must hold the GPU PHYSICAL offset, not a virtual pointer -- exactly what
        // libd3d8's GetGPUAddress() yields: (virtual & (XMETAL_MAX_PHYSICAL_OFFSET-1)).
        // Storing the raw virtual address makes the texture fetch run off the DMA
        // window (blue-screen on HW / DMA-length assert on xemu).
        if (*pData < vidSize)
            *pData = ((DWORD)(vid + *pData)) & (D3D_MAX_PHYSICAL_OFFSET - 1);
        // Keep the runtime from ever freeing a texture we own: clear
        // D3DCOMMON_D3DCREATED (0x2000) and pin a high refcount.
        *(DWORD *)p = (common & ~0x2000u) | 0x0000F000u;
    }
}

UIX_SKIN *UixSkinLoad(const void *pData, DWORD cbData, DWORD LanguageID)
{
    if (!pData || cbData < sizeof(XSK_HEADER))
        return NULL;
    const XSK_HEADER *h = (const XSK_HEADER *)pData;
    if (h->Magic[0] != 'X' || h->Magic[1] != 'S' || h->Magic[2] != 'K' || h->Magic[3] != '0')
        return NULL;
    if (h->RecordSize != sizeof(XSK_SECTION))
        return NULL;
    DWORD tableEnd = sizeof(XSK_HEADER) + (DWORD)h->SectionCount * sizeof(XSK_SECTION);
    if (tableEnd > cbData)
        return NULL;

    UIX_SKIN *s = (UIX_SKIN *)SkAlloc(sizeof(UIX_SKIN));
    if (!s)
        return NULL;
    memset(s, 0, sizeof(*s));
    s->lang = LanguageID;

    // The headers/layouts/strings are CPU-read, so a plain-heap copy suffices; each
    // XPR's pixel data is moved into its own contiguous WC buffer during fixup.
    s->pBase = (BYTE *)SkAlloc(cbData);
    if (!s->pBase) {
        SkFree(s);
        return NULL;
    }
    memcpy(s->pBase, pData, cbData);
    s->cbBase    = cbData;
    s->pSections = (const XSK_SECTION *)(s->pBase + sizeof(XSK_HEADER));
    s->cSections = h->SectionCount;

    // Validate section extents and fix up texture blobs.
    for (DWORD i = 0; i < s->cSections; i++) {
        const XSK_SECTION *sec = &s->pSections[i];
        if ((DWORD)sec->PayloadOffset + sec->PayloadSize > cbData) {
            UixSkinFree(s);
            return NULL;
        }
        if (sec->Kind == XSK_KIND_IMAGE || sec->Kind == XSK_KIND_SCREEN)
            FixupXpr(s, sec);
    }
    return s;
}

void UixSkinFree(UIX_SKIN *s)
{
    if (!s)
        return;
    for (DWORD i = 0; i < s->cVidmem; i++)
        if (s->vidmem[i])
            D3D_FreeContiguousMemory(s->vidmem[i]);
    SkFree(s->pBase);
    SkFree(s);
}

const WCHAR *UixSkinGetString(UIX_SKIN *s, DWORD StringResID)
{
    if (!s) return NULL;
    const BYTE *p = ResolveObject(s, FindSection(s, XSK_KIND_STRING, 0, FALSE), StringResID, NULL);
    if (!p) return NULL;
    // Skip a leading icon block if present.
    if (*(const WORD *)p == XSK_ICON_MARKER) {
        WORD count = *(const WORD *)(p + 2);
        p += 4 + (DWORD)count * sizeof(UIX_SKIN_ICON_INFO);
    }
    return (const WCHAR *)p;
}

HRESULT UixSkinGetLayout(UIX_SKIN *s, DWORD ScreenResID, DWORD ObjectResID,
                         UIX_SKIN_LAYOUT_INFO **ppLayout)
{
    if (ppLayout) *ppLayout = NULL;
    if (!s || !ppLayout) return E_POINTER;
    const XSK_SECTION *sec = FindSection(s, XSK_KIND_SCREEN, ScreenResID, TRUE);
    const BYTE *p = ResolveObject(s, sec, ObjectResID, NULL);
    if (!p) return E_FAIL;
    *ppLayout = (UIX_SKIN_LAYOUT_INFO *)p;   // on-disk record is a 1:1 match
    return S_OK;
}

// Return the (fixed-up) D3DTexture header that sits at a given BYTE offset into a
// section's XPR header array. SkinBld assigns both the layout's ImageOffset and an
// image object's BlobOffset as `slot * sizeof(D3DTexture)` (see XprBuilder), i.e. a
// direct offset into the array that begins right after the 12-byte XPR header.
static HRESULT XprTextureAtOffset(UIX_SKIN *s, const XSK_SECTION *sec, DWORD headerOffset,
                                  IDirect3DTexture8 **ppTexture)
{
    if (ppTexture) *ppTexture = NULL;
    if (!s || !sec || !ppTexture || sec->XprOffset == XSK_ABSENT)
        return E_FAIL;
    BYTE *xpr = (BYTE *)SectionBlob(s, sec) + sec->XprOffset;
    BYTE *hdr = xpr + 12 + headerOffset;     // D3DTexture header (Data already fixed up)
    if (hdr + 20 > s->pBase + s->cbBase)
        return E_FAIL;
    *ppTexture = (IDirect3DTexture8 *)hdr;
    return S_OK;
}

// GetImage: the IMAGE section's object table maps a real resId -> the texture's
// header offset (SkinBld stored slot*sizeof(D3DTexture) as the object BlobOffset).
HRESULT UixSkinGetImage(UIX_SKIN *s, DWORD ImageResID, IDirect3DTexture8 **ppTexture)
{
    if (ppTexture) *ppTexture = NULL;
    const XSK_SECTION *sec = FindSection(s, XSK_KIND_IMAGE, 0, FALSE);
    if (!s || !sec)
        return E_FAIL;
    DWORD blobOff = 0;
    if (!ResolveObject(s, sec, ImageResID, &blobOff))
        return E_FAIL;
    return XprTextureAtOffset(s, sec, blobOff, ppTexture);
}

// GetScreenImage: ImageResID is the layout's ImageOffset -- ALREADY the header-array
// byte offset, not a resId. Resolve it directly against the screen section's XPR.
HRESULT UixSkinGetScreenImage(UIX_SKIN *s, DWORD ScreenResID, DWORD ImageResID,
                              IDirect3DTexture8 **ppTexture)
{
    return XprTextureAtOffset(s, FindSection(s, XSK_KIND_SCREEN, ScreenResID, TRUE),
                              ImageResID, ppTexture);
}

LPCSTR UixSkinGetAudioName(UIX_SKIN *s, DWORD AudioResID)
{
    if (!s) return NULL;
    const BYTE *p = ResolveObject(s, FindSection(s, XSK_KIND_AUDIO, 0, FALSE), AudioResID, NULL);
    if (!p) return NULL;
    const WCHAR *w = (const WCHAR *)p;
    DWORD i = 0;
    for (; w[i] && i + 1 < sizeof(s->audioNarrow); i++)
        s->audioNarrow[i] = (char)(w[i] & 0xFF);
    s->audioNarrow[i] = 0;
    return s->audioNarrow;
}

HRESULT UixSkinGetWordLength(UIX_SKIN *s, LPCWSTR pString, DWORD *pWordLength)
{
    (void)s;
    if (!pWordLength) return E_POINTER;
    DWORD n = 0;
    if (pString)
        while (pString[n] && pString[n] != L' ' && pString[n] != L'\n')
            n++;
    *pWordLength = n;
    return S_OK;
}

// ---------------------------------------------------------- IPluginSupport ---
// Opaque-handle interface: the object handed out is this struct, and the exported
// PluginSupport_* functions (declared in uix.h) cast the handle back to it.

struct RXDK_PLUGIN_SUPPORT {
    LONG      lRefCount;
    UIX_SKIN *pSkin;
};

IPluginSupport *UixCreatePluginSupport(UIX_SKIN *pSkin)
{
    RXDK_PLUGIN_SUPPORT *ps = (RXDK_PLUGIN_SUPPORT *)SkAlloc(sizeof(RXDK_PLUGIN_SUPPORT));
    if (!ps)
        return NULL;
    ps->lRefCount = 1;
    ps->pSkin     = pSkin;
    return (IPluginSupport *)ps;
}

void UixDestroyPluginSupport(IPluginSupport *pSupport)
{
    SkFree(pSupport);
}

static RXDK_PLUGIN_SUPPORT *Ps(PluginSupport *pThis) { return (RXDK_PLUGIN_SUPPORT *)pThis; }

extern "C" {

ULONG WINAPI PluginSupport_AddRef(PluginSupport *pThis)
{
    return (ULONG)(++Ps(pThis)->lRefCount);
}
ULONG WINAPI PluginSupport_Release(PluginSupport *pThis)
{
    RXDK_PLUGIN_SUPPORT *ps = Ps(pThis);
    LONG c = --ps->lRefCount;
    if (c <= 0) { SkFree(ps); return 0; }
    return (ULONG)c;
}
HRESULT WINAPI PluginSupport_GetString(PluginSupport *pThis, DWORD StringResID, LPCWSTR *ppString)
{
    if (!ppString) return E_POINTER;
    *ppString = UixSkinGetString(Ps(pThis)->pSkin, StringResID);
    return *ppString ? S_OK : E_FAIL;
}
HRESULT WINAPI PluginSupport_GetLayout(PluginSupport *pThis, DWORD ScreenResID, DWORD ObjectResID,
                                       UIX_SKIN_LAYOUT_INFO **ppLayout)
{
    return UixSkinGetLayout(Ps(pThis)->pSkin, ScreenResID, ObjectResID, ppLayout);
}
HRESULT WINAPI PluginSupport_GetImage(PluginSupport *pThis, DWORD ImageResID,
                                      IDirect3DTexture8 **ppTexture)
{
    return UixSkinGetImage(Ps(pThis)->pSkin, ImageResID, ppTexture);
}
HRESULT WINAPI PluginSupport_GetScreenImage(PluginSupport *pThis, DWORD ScreenResID,
                                            DWORD ImageResID, IDirect3DTexture8 **ppTexture)
{
    return UixSkinGetScreenImage(Ps(pThis)->pSkin, ScreenResID, ImageResID, ppTexture);
}
HRESULT WINAPI PluginSupport_GetWordLength(PluginSupport *pThis, LPCWSTR pString, DWORD *pWordLength)
{
    return UixSkinGetWordLength(Ps(pThis)->pSkin, pString, pWordLength);
}

} // extern "C"
