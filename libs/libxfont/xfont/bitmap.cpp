//****************************************************************************
//
// XBox bitmap font library
//
// Ported from the leaked Xbox SDK source. Disk-file font loading
// (BP_OpenBitmapFont / BP_GetCharacterDataFromFile, the CreateFileA/ReadFile
// path) is deliberately NOT ported in this pass -- only the memory-loading
// path (BP_OpenBitmapFontFromMemory / BP_GetCharacterDataFromMemory) is kept,
// which is also what backs XFONT_OpenDefaultFont. The BP_Font struct's
// hFile/offset-table fields are simply never populated by this path (the
// memory-loading function already sets hFile = INVALID_HANDLE_VALUE, so the
// shared BP_UnloadFont's guarded CloseHandle call is always skipped) -- no
// struct changes needed, this is a pure function-level trim.
//
// History:
//
//   07/06/00 [andrewso] - Created
//   08/04/00 [andrewso] - Added compressed glyph bitmaps
//
//****************************************************************************

// This is a C++ file so we can take advantage of inheritance to extend
// the font data structure.

#include <xtl.h>
#include "xfont.h"
#include "xfontformat.h"

#include <assert.h>
#include "font.h"

//****************************************************************************
// Definitions.
//****************************************************************************

// Holds the font information.
struct BP_Font : public Font
{

    // If the font file is smaller than the cache size, then we'll just
    // load it into this memory location and look it up directly.
    //
    void *pMemory;
    FontHeader *pHeader;
    DWORD *rgoGlyphs;
    SegmentRun *pSegmentRunTable;
    SegmentDescriptor *pSegmentTable;

    // If the file won't fit into the cache or we've been asked to use
    // minimal memory, then we'll manually munge through the file
    // for every character.
    //
    HANDLE hFile;

    // The location of the tables inside of the file.
    DWORD oGlyphOffsets;
    DWORD oSegmentRunTable;
    DWORD oSegmentTable;

};

// Random forwards.
static void __stdcall BP_UnloadFont(struct BP_Font *);
static HRESULT __stdcall BP_ResetTransform(struct BP_Font *);
static HRESULT __fastcall BP_GetCharacterDataFromMemory(struct BP_Font *, WCHAR, struct _Glyph **, unsigned *);

//****************************************************************************
// APIs.
//****************************************************************************

//============================================================================
// Open a bitmap font from a block of memory.
//============================================================================

HRESULT __stdcall BP_OpenBitmapFontFromMemory
(
    CONST void *pFontData,
    unsigned uFontDataSize,
    struct _Font **ppFont
)
{
    HRESULT hr;
    BP_Font *pFont;
    FontHeader *pHeader;

    // Allocate the memory for the font structure and zero it.
    pFont = (BP_Font *)malloc(sizeof(BP_Font));

    if (!pFont)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pFont, sizeof(BP_Font));

    pFont->hFile = INVALID_HANDLE_VALUE;

    // Get the header.
    pHeader = (FontHeader *)pFontData;

    // Do a sanity check.
    if (pHeader->wSignature != (WORD)'XFNT')
    {
        hr = E_FAIL;
        goto Error;
    }

    if (pHeader->wVersion != FILE_VERSION)
    {
        hr = E_FAIL;
        goto Error;
    }

    // Set up the tables.
    pFont->rgoGlyphs = (DWORD *)(pHeader + 1);
    pFont->pSegmentRunTable = (SegmentRun *)(pFont->rgoGlyphs + pHeader->cGlyphs);
    pFont->pSegmentTable = (SegmentDescriptor *)(pFont->pSegmentRunTable + pHeader->cSegmentRunTable);

    // Save font info.
    pFont->uCellHeight = pHeader->uCellHeight;
    pFont->uDescent = pHeader->uDescent;
    pFont->uAntialiasLevel = pHeader->uAntialiasLevel;
    pFont->uRLEWidth = pHeader->uRLEWidth;
    pFont->uMaxBitmapHeight = pHeader->uMaxBitmapHeight;
    pFont->uMaxBitmapWidth = pHeader->uMaxBitmapWidth;
    pFont->uReferenceCount = 1;

    // So we'll get the data the right way.
    pFont->pfnGetCharacterData = (CB_GetCharacterData)BP_GetCharacterDataFromMemory;

    // Set up the callbacks.
    pFont->pfnUnloadFont = (CB_UnloadFont)BP_UnloadFont;
    pFont->pfnResetTransform = (CB_ResetTransform)BP_ResetTransform;

    pFont->pHeader = pHeader;

    // Return it.
    *ppFont = (Font *)pFont;

    return NOERROR;

Error:
    BP_UnloadFont(pFont);

    return hr;
}

//****************************************************************************
// Callbacks to store in the Font structure.
//****************************************************************************

//============================================================================
// Free all memory associated with the font.
//============================================================================

static void __stdcall BP_UnloadFont
(
    struct BP_Font *pFont
)
{
    if (pFont->hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pFont->hFile);
    }

    free(pFont->pMemory);
    free(pFont);
}

//============================================================================
// Reset the size, alpha, etc of the font.
//============================================================================

static HRESULT __stdcall BP_ResetTransform
(
    struct BP_Font *pFont
)
{
    // Can't reset a bitmap font.
    return E_FAIL;
}

//============================================================================
// Get the data and bitmap information for one character.
//============================================================================

static HRESULT __fastcall BP_GetCharacterDataFromMemory
(
    BP_Font *pFont,
    WCHAR wch,
    struct _Glyph **ppGlyph,
    unsigned *pcbGlyphSize
)
{
    HRESULT hr;

    SegmentDescriptor *pSegment;
    unsigned iGlyphData;
    unsigned uMask;

    // Break the character up.
    unsigned uSegment = CHAR_SEGMENT(wch);
    unsigned uOffset = CHAR_OFFSET(wch);

    // Find its segment in the segment run table.
    SegmentRun *pRun = pFont->pSegmentRunTable;
    SegmentRun *pRunMax = pRun + pFont->pHeader->cSegmentRunTable;

    for (;;)
    {
        if (pRun == pRunMax)
        {
            goto NotFound;
        }

        if (uSegment >= pRun->wFirstSegment && uSegment < (unsigned)pRun->wFirstSegment + pRun->cSegments)
        {
            break;
        }

        pRun++;
    }

    // Get the segment.
    pSegment = pFont->pSegmentTable + pRun->iSegmentTable + uSegment - pRun->wFirstSegment;

    // Calculate the glyph data index.
    iGlyphData = pSegment->iGlyph;
    uMask = pSegment->wCharMask;

    if (!(uMask & (1 << uOffset)))
    {
        goto NotFound;
    }

    // Mask all the bits under this one.
    uMask &= (1 << uOffset) - 1;

    // Count them.
    while (uMask)
    {
        iGlyphData++;

        uMask &= uMask - 1;
    }

    // We have a winner.
    *ppGlyph = (Glyph *)((BYTE *)pFont->pHeader + pFont->rgoGlyphs[iGlyphData]);
    *pcbGlyphSize = pFont->rgoGlyphs[iGlyphData + 1] - pFont->rgoGlyphs[iGlyphData];

    return NOERROR;

NotFound:
    // Recurse with the default character.
    if (wch != pFont->pHeader->wDefaultChar)
    {
        hr = BP_GetCharacterDataFromMemory(pFont, pFont->pHeader->wDefaultChar, ppGlyph, pcbGlyphSize);

        if (FAILED(hr))
        {
            return hr;
        }
        else
        {
            return S_FALSE;
        }
    }

    return E_FAIL;
}
