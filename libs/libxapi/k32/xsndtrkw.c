#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Soundtrack WRITE APIs - XAddSoundtrack / XAddSongToSoundtrack - the companion
 * to the read side in xsndtrk.c. Produces the on-disk ST.DB format that
 * xsndtrk.c decodes:
 *
 *     page 0                            STDB_HDR
 *     pages 1 .. MAX_SOUNDTRACKS        STDB_STDESC descriptors (one per slot)
 *     pages MAX_SOUNDTRACKS+1 ..        STDB_LIST song-list blocks
 *
 * so a soundtrack written here reads back through XFindFirstSoundtrack /
 * XGetSoundtrackSongInfo and interoperates with the dashboard.
 *
 * This lives in its own translation unit on purpose. XAddSongToSoundtrack drives
 * the WMA file decoder (WmaCreateDecoderEx, libdsound) to obtain the song's play
 * length and - when the caller passes no name - its Title tag. A title that uses
 * the write side therefore links libdsound too; a title that uses only the read
 * side never pulls this object and never needs libdsound. The internal
 * block-manager helpers (XapipSoundtrackSeek*, Xapip{Write,Update,Find,New}*,
 * XapiBeginUsingSoundtracks / XapiEndUsingSoundtracks) manage the ST.DB pages.
 */

#include "basedll.h"
#include <xboxp.h>

//
// From the read side (xsndtrk.c): the ST.DB path and the read-cache lookasides
// the write path invalidates after mutating the database, plus its page reader.
//
extern const OBJECT_STRING XapiStDbFile;
extern STDB_LIST   XapiListLookaside;
extern STDB_STDESC XapiStLookaside;
extern BOOL XapiReadFromStDb(HANDLE Handle, PBYTE Buffer, DWORD Signature);

#define STDB_MUSICDIR "\\Device\\Harddisk0\\partition1\\TDATA\\FFFE0000\\MUSIC\\"

#ifndef FILE_CREATED
#define FILE_CREATED 0x00000002     // NtCreateFile IoStatusBlock.Information
#endif

//
// Constants read straight out of soundtrack.obj:
//   - the DB-init image is one XMemAlloc of (1 + MAX_SOUNDTRACKS) pages with
//     these exact attribute bits (retail: XMemAlloc(0xCA00, 0x64830000));
//   - the song copy streams through a 64 KiB MmAllocateSystemMemory granule
//     (retail: MmAllocateSystemMemory(0x10000, PAGE_READWRITE));
//   - the progress routine follows CopyFileEx's contract: 0 = continue,
//     3 (PROGRESS_QUIET) = stop notifying, anything else = abort.
//
#define SOUNDTRACK_INIT_XMEM_ATTR   0x64830000
#define SOUNDTRACK_COPY_CHUNK       0x10000
#define SOUNDTRACK_PROGRESS_QUIET   3

//
// Minimal declaration of the libdsound WMA file decoder, imported the way
// retail's soundtrack.obj imports it: the WmaCreateDecoderEx factory plus the
// returned object's vtable. We cannot #include <dsound.h> here -- libxapi's
// freestanding C environment lacks the objbase-level STDAPI_ / interface macros
// that header's free-function prototypes need. The layout below mirrors
// shared/include/dsound.h exactly (WMAXMOFileContDesc is pack(1); the vtable
// order puts Release at slot +0x4, GetFileHeader at +0x28 and
// GetFileContentDescription at +0x2C -- the very slots retail calls), so the ABI
// is identical. STDMETHODCALLTYPE / STDAPICALLTYPE are __stdcall on this target.
//
typedef struct _WMAXMOFileHeader {
    DWORD dwVersion;
    DWORD dwSampleRate;
    DWORD dwNumChannels;
    DWORD dwDuration;
    DWORD dwBitrate;
} WMAXMOFileHeader;

#pragma pack(push, 1)
typedef struct _WMAXMOFileContDesc {
    WORD    wTitleLength;       // in: buffer size in BYTES / out: unchanged
    WORD    wAuthorLength;
    WORD    wCopyrightLength;
    WORD    wDescriptionLength;
    WORD    wRatingLength;
    WCHAR * pTitle;             // in: caller buffer; the tag is copied into it
    WCHAR * pAuthor;
    WCHAR * pCopyright;
    WCHAR * pDescription;
    WCHAR * pRating;
} WMAXMOFileContDesc;
#pragma pack(pop)

typedef struct XWmaFileMediaObject XWmaFileMediaObject;

typedef struct XWmaFileMediaObjectVtbl {
    ULONG   (__attribute__((__stdcall__)) *AddRef)(XWmaFileMediaObject *);
    ULONG   (__attribute__((__stdcall__)) *Release)(XWmaFileMediaObject *);
    HRESULT (__attribute__((__stdcall__)) *GetInfo)(XWmaFileMediaObject *, PVOID);
    HRESULT (__attribute__((__stdcall__)) *GetStatus)(XWmaFileMediaObject *, LPDWORD);
    HRESULT (__attribute__((__stdcall__)) *Process)(XWmaFileMediaObject *, const void *, const void *);
    HRESULT (__attribute__((__stdcall__)) *Discontinuity)(XWmaFileMediaObject *);
    HRESULT (__attribute__((__stdcall__)) *Flush)(XWmaFileMediaObject *);
    HRESULT (__attribute__((__stdcall__)) *Seek)(XWmaFileMediaObject *, LONG, DWORD, LPDWORD);
    HRESULT (__attribute__((__stdcall__)) *GetLength)(XWmaFileMediaObject *, LPDWORD);
    VOID    (__attribute__((__stdcall__)) *DoWork)(XWmaFileMediaObject *);
    HRESULT (__attribute__((__stdcall__)) *GetFileHeader)(XWmaFileMediaObject *, WMAXMOFileHeader *);
    HRESULT (__attribute__((__stdcall__)) *GetFileContentDescription)(XWmaFileMediaObject *, WMAXMOFileContDesc *);
    HRESULT (__attribute__((__stdcall__)) *SeekToTime)(XWmaFileMediaObject *, DWORD, LPDWORD);
} XWmaFileMediaObjectVtbl;

struct XWmaFileMediaObject {
    const XWmaFileMediaObjectVtbl *lpVtbl;
};

//
// libdsound factory (retail imports _WmaCreateDecoderEx@32). The 7th argument is
// LPWAVEFORMATEX; we pass NULL (we need only the header + content description,
// not the decoded PCM format) and type it PVOID to avoid the waveform headers.
//
extern HRESULT __attribute__((__stdcall__)) WmaCreateDecoderEx(
    LPCSTR pszFileName, HANDLE hFile, BOOL fAsyncMode,
    DWORD dwLookaheadBufferSize, DWORD dwMaxPackets, DWORD dwYieldRate,
    PVOID pwfxDecoded, XWmaFileMediaObject **ppMediaObject);

//
// Serializes all soundtrack-database mutation. The dashboard and other title
// threads share ST.DB, so writers hold this across the whole open/modify/close
// (retail: the statically-initialized _XSoundtrackCriticalSection).
//
static RTL_CRITICAL_SECTION XSoundtrackCriticalSection;
static LONG                 XSoundtrackCsInit = 0;

static VOID XapiEnsureSoundtrackCs(VOID)
{
    //
    // 0 = uninitialised, 1 = initialising, 2 = ready. The winner initialises and
    // publishes 2; any racing loser waits for it rather than entering an
    // uninitialised critical section.
    //
    if (XSoundtrackCsInit == 2)
        return;
    if (InterlockedCompareExchange(&XSoundtrackCsInit, 1, 0) == 0)
    {
        RtlInitializeCriticalSection(&XSoundtrackCriticalSection);
        XSoundtrackCsInit = 2;
    }
    else
    {
        while (XSoundtrackCsInit != 2)
            ;   // brief: the winner is mid-RtlInitializeCriticalSection
    }
}

//
// Page seek helpers, matching the read side's page math and the retail statics
// XapipSoundtrackSeek / XapipSoundtrackSeekToListSegment:
//   descriptor page for soundtrack slot i  -> (i + 1) * STDB_PAGE_SIZE
//   list-block page for list index n        -> (n + 1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE
//
static BOOL XapipSoundtrackSeek(HANDLE h, UINT slot)
{
    return SetFilePointer(h, (slot + 1) * STDB_PAGE_SIZE, NULL, FILE_BEGIN)
           != INVALID_SET_FILE_POINTER;
}

static BOOL XapipSoundtrackSeekToListSegment(HANDLE h, UINT listBlock)
{
    return SetFilePointer(h, (listBlock + 1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE,
                          NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

//
// Write the 512-byte page at the current file position (retail
// XapipWriteToSoundtrackDb).
//
static BOOL XapipWriteToSoundtrackDb(HANDLE h, PVOID page)
{
    DWORD n;
    return WriteFile(h, page, STDB_PAGE_SIZE, &n, NULL) && n == STDB_PAGE_SIZE;
}

//
// Persist the header (page 0) from the in-memory copy (retail
// XapipUpdateSoundtrackDbHeader: header bytes copied into the 512-byte page,
// seek 0, write). Only the first sizeof(STDB_HDR) bytes of page 0 are ever read
// back, so the tail is don't-care; we zero it for cleanliness.
//
static BOOL XapipUpdateSoundtrackDbHeader(HANDLE h, PSTDB_HDR pHeader)
{
    BYTE page[STDB_PAGE_SIZE];
    RtlZeroMemory(page, sizeof(page));
    RtlCopyMemory(page, pHeader, sizeof(STDB_HDR));
    return SetFilePointer(h, 0, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER
           && XapipWriteToSoundtrackDb(h, page);
}

//
// Linear scan of the header's StBlocks[] for a soundtrack Id (retail
// XapipFindSoundtrackBlock). Returns the slot index, or MAX_SOUNDTRACKS if the
// Id is not present.
//
static UINT XapipFindSoundtrackBlock(PSTDB_HDR pHeader, UINT SoundtrackId)
{
    UINT i;
    for (i = 0; i < MAX_SOUNDTRACKS; i++)
        if (pHeader->StBlocks[i] == SoundtrackId)
            break;
    return i;
}

//
// Open (creating if absent) the soundtrack database read/write and read its
// header. A freshly created database is a header page followed by
// MAX_SOUNDTRACKS zeroed descriptor pages (no list blocks yet) -- exactly what
// the read side expects. Also ensures the MUSIC directory exists. Caller holds
// the CS. Retail: XapiBeginUsingSoundtracks.
//
static NTSTATUS XapiBeginUsingSoundtracks(HANDLE *phFile, PSTDB_HDR pHeader)
{
    NTSTATUS          status;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK   iosb;
    ANSI_STRING       as;
    HANDLE            hDir, hFile;
    BYTE              page[STDB_PAGE_SIZE];

    //
    // Ensure the MUSIC directory exists (retail creates it and tolerates
    // "already there"). FILE_ATTRIBUTE_NORMAL stands in for retail's literal
    // 0x88 -- the extra bit is a no-op for a directory create, whose
    // directory-ness comes from FILE_DIRECTORY_FILE.
    //
    RtlInitAnsiString(&as, STDB_MUSICDIR);
    InitializeObjectAttributes(&obja, (POBJECT_STRING)&as, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtCreateFile(&hDir, SYNCHRONIZE | FILE_LIST_DIRECTORY, &obja, &iosb, NULL,
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_CREATE,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                              | FILE_OPEN_FOR_BACKUP_INTENT);
    if (NT_SUCCESS(status))
        NtClose(hDir);
    else if (status != STATUS_OBJECT_NAME_COLLISION)
        return status;

    //
    // Open/create the database. GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE|DELETE
    // matches retail (0xC0110000); the DELETE right lets us discard a
    // half-created file if init fails.
    //
    InitializeObjectAttributes(&obja, (POBJECT_STRING)&XapiStDbFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtCreateFile(&hFile, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE,
                          &obja, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status))
        return status;

    if (iosb.Information == FILE_CREATED)
    {
        //
        // New database: lay down the header + MAX_SOUNDTRACKS empty descriptor
        // pages as one zeroed image (retail allocates it with XMemAlloc), set
        // the header Version, write it all, then rewind. On failure, discard the
        // half-created file via its DELETE right.
        //
        ULONG  cb = (1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE;
        PBYTE  p  = (PBYTE)XMemAlloc(cb, SOUNDTRACK_INIT_XMEM_ATTR);
        DWORD  written;

        if (p == NULL)
        {
            BYTE del = TRUE;
            NtSetInformationFile(hFile, &iosb, &del, sizeof(del), FileDispositionInformation);
            NtClose(hFile);
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory(p, cb);
        ((PSTDB_HDR)p)->Version = STDB_VERSION;

        if (!WriteFile(hFile, p, cb, &written, NULL) || written != cb)
        {
            BYTE del = TRUE;
            XMemFree(p, SOUNDTRACK_INIT_XMEM_ATTR);
            NtSetInformationFile(hFile, &iosb, &del, sizeof(del), FileDispositionInformation);
            NtClose(hFile);
            return STATUS_UNSUCCESSFUL;
        }
        XMemFree(p, SOUNDTRACK_INIT_XMEM_ATTR);
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    }

    //
    // Read the header page. XapiReadFromStDb reads a full STDB_PAGE_SIZE page, so
    // it must land in a page-sized buffer; copy the header out of it (retail
    // reads into its 512-byte scratch page and copies sizeof(STDB_HDR) into the
    // header cache -- reading straight into the 416-byte STDB_HDR would overrun).
    //
    if (!XapiReadFromStDb(hFile, page, 0))
    {
        NtClose(hFile);
        return STATUS_UNSUCCESSFUL;
    }
    RtlCopyMemory(pHeader, page, sizeof(STDB_HDR));

    *phFile = hFile;
    return STATUS_SUCCESS;
}

//
// Close the database and invalidate the read caches so a subsequent query
// re-reads the mutated database (retail XapiEndUsingSoundtracks). All writes are
// write-through; nothing to flush.
//
static VOID XapiEndUsingSoundtracks(HANDLE hFile)
{
    XapiListLookaside.StId = INVALID_STID;
    XapiStLookaside.Id     = INVALID_STID;
    if (hFile)
        NtClose(hFile);
}

//
// Allocate the next monotonic song ID for a soundtrack (retail
// XapipSoundtrackNewSongID). The ID packs the soundtrack Id in the high word and
// a per-database sequence in the low word -- exactly what the read side decodes
// as MUSIC\%04x\%08x.WMA. NextSongId is persisted immediately.
//
static UINT XapipSoundtrackNewSongID(HANDLE h, PSTDB_HDR pHeader, UINT SoundtrackId)
{
    UINT seq = pHeader->NextSongId;
    pHeader->NextSongId = seq + 1;

    if (!XapipUpdateSoundtrackDbHeader(h, pHeader))
        return INVALID_STID;

    return (SoundtrackId << 16) | (seq & 0xFFFF);
}

//
// Find a reusable (InUse == 0) list block, or append a fresh one at end of file
// and return its index (retail XapipSoundtrackFindNewListBlock). The on-disk
// page for index n is (n + 1 + MAX_SOUNDTRACKS).
//
static UINT XapipSoundtrackFindNewListBlock(HANDLE h)
{
    BYTE page[STDB_PAGE_SIZE];
    UINT n;

    for (;;)
    {
        if (!XapipSoundtrackSeekToListSegment(h, 0))
            return INVALID_STID;

        for (n = 0; ; n++)
        {
            PSTDB_LIST pList = (PSTDB_LIST)page;

            if (!XapiReadFromStDb(h, page, STDB_LISTSIG))
            {
                //
                // Past EOF or a bad signature: append a fresh empty block and
                // rescan so its index is the one returned (retail rescans from
                // the top after each append).
                //
                RtlZeroMemory(page, sizeof(page));
                pList->Signature = STDB_LISTSIG;
                pList->InUse = FALSE;
                if (SetFilePointer(h, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER ||
                    !XapipWriteToSoundtrackDb(h, page))
                    return INVALID_STID;
                break;      // rescan from the top
            }

            if (!pList->InUse)
                return n;   // reuse this free block
        }
    }
}

//
// Read the source WMA's play length (milliseconds) and, when the caller passed
// no name, its Title tag, through the file decoder -- retail drives the same
// decoder object, taking the length from GetFileHeader (its vtable +0x28) and
// the title from GetFileContentDescription (+0x2C). The decoder borrows the
// supplied handle (CWmaMediaObject::InitializeFile leaves m_fCloseFile FALSE for
// a handle it did not open), so the caller keeps ownership. Returns the decoder
// HRESULT; on success *pSongLen holds the length and, if wantName, nameBuf holds
// the title (or L"Unknown").
//
static HRESULT XapiWmaSongInfo(HANDLE hSrc, UINT *pSongLen,
                               PWSTR nameBuf, DWORD cchNameBuf, BOOL wantName)
{
    XWmaFileMediaObject *pDecoder = NULL;
    WMAXMOFileHeader     fileHeader;
    HRESULT              hr;

    // NULL pwfxDecoded: we need the ASF header + content tags, not the decoded
    // PCM format, so we skip the format query (and its WAVEFORMATEX dependency).
    hr = WmaCreateDecoderEx(NULL, hSrc, FALSE, 0, 0, 0, NULL, &pDecoder);
    if (FAILED(hr))
        return hr;

    RtlZeroMemory(&fileHeader, sizeof(fileHeader));
    hr = pDecoder->lpVtbl->GetFileHeader(pDecoder, &fileHeader);
    if (SUCCEEDED(hr))
    {
        *pSongLen = fileHeader.dwDuration;

        if (wantName)
        {
            //
            // GetFileContentDescription is IN/OUT: hand it the name buffer and
            // its size in BYTES and it copies the Title tag in (empty string if
            // the file carries none). Retail then defaults an empty title to
            // "Unknown".
            //
            WMAXMOFileContDesc desc;
            RtlZeroMemory(&desc, sizeof(desc));
            desc.pTitle       = nameBuf;
            desc.wTitleLength = (WORD)(cchNameBuf * sizeof(WCHAR));
            nameBuf[0]        = L'\0';

            hr = pDecoder->lpVtbl->GetFileContentDescription(pDecoder, &desc);
            if (SUCCEEDED(hr) && nameBuf[0] == L'\0')
                lstrcpynW(nameBuf, L"Unknown", cchNameBuf);
        }
    }

    pDecoder->lpVtbl->Release(pDecoder);
    return hr;
}

//
// Copy the source file into the (already created, pre-sized) destination handle,
// reporting progress (retail XapipCopySongToMusicDirectory). The source pointer
// is reset first: the decoder above left it inside the WMA header, and the copy
// streams sequentially from the start. Returns S_OK or an HRESULT.
//
static HRESULT XapipCopySongToMusicDirectory(HANDLE hSrc, HANDLE hDst,
                                             PCWSTR pszSoundtrack, PCWSTR pszSong,
                                             LP_SOUNDTRACK_PROGRESS_ROUTINE lpRoutine,
                                             LPVOID Context)
{
    PBYTE           buf;
    DWORD           read;
    LARGE_INTEGER   total, done, zero;
    BOOL            report = TRUE;

    if (!GetFileSizeEx(hSrc, &total))
        return HRESULT_FROM_WIN32(GetLastError());

    buf = (PBYTE)MmAllocateSystemMemory(SOUNDTRACK_COPY_CHUNK, PAGE_READWRITE);
    if (buf == NULL)
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

    zero.QuadPart = 0;
    SetFilePointerEx(hSrc, zero, NULL, FILE_BEGIN);
    done.QuadPart = 0;

    if (lpRoutine)
    {
        DWORD r = lpRoutine(pszSoundtrack, pszSong, total, done, Context);
        if (r == SOUNDTRACK_PROGRESS_QUIET)
            report = FALSE;
        else if (r != 0)
        {
            MmFreeSystemMemory(buf, SOUNDTRACK_COPY_CHUNK);
            return HRESULT_FROM_WIN32(ERROR_REQUEST_ABORTED);
        }
    }

    while (done.QuadPart < total.QuadPart)
    {
        DWORD written;
        if (!ReadFile(hSrc, buf, SOUNDTRACK_COPY_CHUNK, &read, NULL) || read == 0)
            break;
        if (!WriteFile(hDst, buf, read, &written, NULL) || written != read)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            MmFreeSystemMemory(buf, SOUNDTRACK_COPY_CHUNK);
            return hr;
        }
        done.QuadPart += read;

        if (report && lpRoutine)
        {
            DWORD r = lpRoutine(pszSoundtrack, pszSong, total, done, Context);
            if (r == SOUNDTRACK_PROGRESS_QUIET)
                report = FALSE;
            else if (r != 0)
            {
                MmFreeSystemMemory(buf, SOUNDTRACK_COPY_CHUNK);
                return HRESULT_FROM_WIN32(ERROR_REQUEST_ABORTED);
            }
        }
    }

    MmFreeSystemMemory(buf, SOUNDTRACK_COPY_CHUNK);
    return S_OK;
}

DWORD
__attribute__((__stdcall__))
XAddSoundtrack(
    IN  LPCWSTR pszSoundtrackName,
    OUT PUINT   pdwSoundtrackId
    )
{
    NTSTATUS  status;
    HANDLE    hFile = NULL;
    STDB_HDR  header;
    STDB_STDESC desc;
    UINT      newId, slot;
    DWORD     rv = 0;

    XapiEnsureSoundtrackCs();
    RtlEnterCriticalSection(&XSoundtrackCriticalSection);

    status = XapiBeginUsingSoundtracks(&hFile, &header);
    if (!NT_SUCCESS(status))
    {
        RtlLeaveCriticalSection(&XSoundtrackCriticalSection);
        XapiSetLastNTError(status);
        return 0;
    }

    if (header.StCount >= MAX_SOUNDTRACKS)
    {
        SetLastError(ERROR_CANNOT_MAKE);
        goto Exit;
    }

    slot  = header.StCount;             // positional descriptor slot (page slot+1)
    newId = header.NextStId;
    header.NextStId = newId + 1;

    //
    // Build the descriptor.
    //
    RtlZeroMemory(&desc, sizeof(desc));
    desc.Signature        = STDB_STSIG;
    desc.Id               = newId;
    desc.SongCount        = 0;
    desc.SoundtrackLength = 0;
    lstrcpynW(desc.Name, pszSoundtrackName, MAX_SOUNDTRACK_NAME);

    header.StBlocks[slot] = newId;      // StBlocks[i] holds the Id at page i+1
    header.StCount        = slot + 1;

    if (pdwSoundtrackId)
        *pdwSoundtrackId = newId;

    //
    // Create the per-soundtrack song directory MUSIC\%04x (named by Id, matching
    // HIWORD of every song ID stored under it). Retail creates it here with
    // FILE_CREATE and tolerates a pre-existing directory.
    //
    {
        CHAR              dir[MAX_PATH];
        ANSI_STRING       as;
        OBJECT_ATTRIBUTES obja;
        IO_STATUS_BLOCK   iosb;
        HANDLE            hDir;
        NTSTATUS          s;

        _snprintf(dir, sizeof(dir), "%s%04x", STDB_MUSICDIR, newId);
        RtlInitAnsiString(&as, dir);
        InitializeObjectAttributes(&obja, (POBJECT_STRING)&as, OBJ_CASE_INSENSITIVE, NULL, NULL);
        s = NtCreateFile(&hDir, SYNCHRONIZE | FILE_LIST_DIRECTORY, &obja, &iosb, NULL,
                         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                             | FILE_OPEN_FOR_BACKUP_INTENT);
        if (NT_SUCCESS(s))
            NtClose(hDir);
        else if (s != STATUS_OBJECT_NAME_COLLISION)
        {
            XapiSetLastNTError(s);
            rv = 0;
            goto Exit;
        }
    }

    //
    // Write the descriptor page BEFORE the header, so the header's
    // StCount/StBlocks never point at a descriptor that was not written.
    //
    {
        BYTE page[STDB_PAGE_SIZE];
        RtlZeroMemory(page, sizeof(page));
        RtlCopyMemory(page, &desc, sizeof(desc));
        if (!XapipSoundtrackSeek(hFile, slot) || !XapipWriteToSoundtrackDb(hFile, page))
        {
            XapiSetLastNTError(STATUS_UNSUCCESSFUL);
            rv = 0;
            goto Exit;
        }
    }

    if (!XapipUpdateSoundtrackDbHeader(hFile, &header))
    {
        XapiSetLastNTError(STATUS_UNSUCCESSFUL);
        rv = 0;
        goto Exit;
    }

    rv = 1;     // success

Exit:
    XapiEndUsingSoundtracks(hFile);
    RtlLeaveCriticalSection(&XSoundtrackCriticalSection);
    return rv;
}

HRESULT
__attribute__((__stdcall__))
XAddSongToSoundtrack(
    IN  UINT    dwSoundtrackId,
    IN  LPCSTR  pszSongPath,
    IN  LPCWSTR pszSongName OPTIONAL,
    IN  LP_SOUNDTRACK_PROGRESS_ROUTINE lpRoutine OPTIONAL,
    IN  LPVOID  Context OPTIONAL,
    OUT PUINT   pdwSongId OPTIONAL
    )
{
    HRESULT     hr = S_OK;
    HANDLE      hFile = NULL, hSrc = INVALID_HANDLE_VALUE, hDst = INVALID_HANDLE_VALUE;
    STDB_HDR    header;
    STDB_STDESC desc;
    BYTE        listPage[STDB_PAGE_SIZE];
    PSTDB_LIST  pList = (PSTDB_LIST)listPage;
    UINT        i, slot, listIndex, listBlock, songId, songLen = 0;
    WCHAR       nameBuf[MAX_SONG_NAME];
    PCWSTR      pName;
    NTSTATUS    status;

    XapiEnsureSoundtrackCs();
    RtlEnterCriticalSection(&XSoundtrackCriticalSection);

    status = XapiBeginUsingSoundtracks(&hFile, &header);
    if (!NT_SUCCESS(status))
    {
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));
        goto Exit;
    }

    //
    // Locate the soundtrack by Id and read its descriptor.
    //
    i = XapipFindSoundtrackBlock(&header, dwSoundtrackId);
    if (i >= header.StCount || i >= MAX_SOUNDTRACKS)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    {
        BYTE page[STDB_PAGE_SIZE];
        if (!XapipSoundtrackSeek(hFile, i) || !XapiReadFromStDb(hFile, page, STDB_STSIG))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
            goto Exit;
        }
        RtlCopyMemory(&desc, page, sizeof(desc));
    }

    if (desc.SongCount >= MAX_SONGS_IN_SNDTRK)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
        goto Exit;
    }

    //
    // Open the source WMA and derive its length + (default) name through the
    // decoder, exactly as retail does.
    //
    hSrc = CreateFileA(pszSongPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hSrc == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hr = XapiWmaSongInfo(hSrc, &songLen, nameBuf, MAX_SONG_NAME, pszSongName == NULL);
    if (FAILED(hr))
        goto Exit;

    pName = pszSongName ? pszSongName : nameBuf;

    //
    // Allocate a song ID and its slot within a list block.
    //
    songId = XapipSoundtrackNewSongID(hFile, &header, dwSoundtrackId);
    if (songId == INVALID_STID)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
        goto Exit;
    }
    if (pdwSongId)
        *pdwSongId = songId;

    slot      = desc.SongCount % WMADESC_LIST_SIZE;
    listIndex = desc.SongCount / WMADESC_LIST_SIZE;

    //
    // Create the destination file, pre-sized to the source. DELETE in the access
    // mask (retail 0x40110000) lets the failure path discard a partial copy.
    //
    {
        CHAR              dst[MAX_PATH];
        ANSI_STRING       as;
        OBJECT_ATTRIBUTES obja;
        IO_STATUS_BLOCK   iosb;
        LARGE_INTEGER     srcSize;

        _snprintf(dst, sizeof(dst), "%s%04x\\%08x.wma", STDB_MUSICDIR, HIWORD(songId), songId);
        RtlInitAnsiString(&as, dst);
        InitializeObjectAttributes(&obja, (POBJECT_STRING)&as, OBJ_CASE_INSENSITIVE, NULL, NULL);

        GetFileSizeEx(hSrc, &srcSize);

        status = NtCreateFile(&hDst, GENERIC_WRITE | SYNCHRONIZE | DELETE, &obja, &iosb, &srcSize,
                              FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                                  | FILE_SEQUENTIAL_ONLY);
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));
            hDst = INVALID_HANDLE_VALUE;
            goto Exit;
        }
    }

    hr = XapipCopySongToMusicDirectory(hSrc, hDst, desc.Name, pName, lpRoutine, Context);
    if (FAILED(hr))
        goto Exit;

    //
    // Update descriptor: bump length + song count, allocate a list block on a new
    // group boundary (slot 0), else reuse the group's existing block.
    //
    desc.SoundtrackLength += songLen;
    desc.SongCount        += 1;

    if (slot == 0)
    {
        listBlock = XapipSoundtrackFindNewListBlock(hFile);
        if (listBlock == INVALID_STID)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
            goto Exit;
        }
        desc.ListBlocks[listIndex] = listBlock;
    }
    else
    {
        listBlock = desc.ListBlocks[listIndex];
    }

    //
    // Write the descriptor page.
    //
    {
        BYTE page[STDB_PAGE_SIZE];
        RtlZeroMemory(page, sizeof(page));
        RtlCopyMemory(page, &desc, sizeof(desc));
        if (!XapipSoundtrackSeek(hFile, i) || !XapipWriteToSoundtrackDb(hFile, page))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
            goto Exit;
        }
    }

    //
    // Read the list block, insert the song, write it back.
    //
    if (!XapipSoundtrackSeekToListSegment(hFile, listBlock) ||
        !XapiReadFromStDb(hFile, listPage, STDB_LISTSIG))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
        goto Exit;
    }

    if (slot == 0)
    {
        RtlZeroMemory(listPage, sizeof(listPage));
        pList->Signature = STDB_LISTSIG;
        pList->StId      = dwSoundtrackId;
        pList->ListIndex = listIndex;
        pList->InUse     = TRUE;
    }

    pList->SongIds[slot]     = songId;
    pList->SongLengths[slot] = songLen;
    lstrcpynW(pList->SongNames[slot], pName, MAX_SONG_NAME);

    if (!XapipSoundtrackSeekToListSegment(hFile, listBlock) ||
        !XapipWriteToSoundtrackDb(hFile, listPage))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
        goto Exit;
    }

    hr = S_OK;

Exit:
    if (hSrc != INVALID_HANDLE_VALUE)
        CloseHandle(hSrc);
    if (hDst != INVALID_HANDLE_VALUE)
    {
        if (FAILED(hr))
        {
            IO_STATUS_BLOCK iosb;
            BYTE del = TRUE;
            NtSetInformationFile(hDst, &iosb, &del, sizeof(del), FileDispositionInformation);
        }
        NtClose(hDst);
    }
    XapiEndUsingSoundtracks(hFile);
    RtlLeaveCriticalSection(&XSoundtrackCriticalSection);
    return hr;
}
