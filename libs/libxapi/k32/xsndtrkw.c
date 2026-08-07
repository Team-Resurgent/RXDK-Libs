#include "bridge_k32.h"
/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    xsndtrkw.c

Abstract:

    XTL Soundtrack WRITE APIs -- XAddSoundtrack / XAddSongToSoundtrack.

    The companion to the read side in xsndtrk.c. Recovered from the retail
    soundtrack.obj disassembly (see docs/5849-xbox-h-recovery.md) and written in
    the read side's clean-room style: it produces the exact on-disk ST.DB format
    xsndtrk.c already decodes (page 0 = STDB_HDR; pages 1..MAX_SOUNDTRACKS =
    STDB_STDESC descriptors; pages MAX_SOUNDTRACKS+1.. = STDB_LIST list blocks),
    so a soundtrack written here reads back through XFindFirstSoundtrack /
    XGetSoundtrackSongInfo and interoperates with the dashboard.

    Caveat: the per-song play length retail obtains from its WMA decoder is not
    reachable here -- the public XMediaObject interface carries no duration
    (XMEDIAINFO has only buffer sizes), and pulling in the libdsound decoder from
    libxapi would invert the library layering. So the length is parsed
    self-contained from the source WMA's ASF "Play Duration" header and stored in
    milliseconds, keeping this whole unit dependency-free. That single metadata
    field is the only part not verified byte-for-byte against retail; the database
    structure is verified through the read side.

--*/

#include "basedll.h"
#include <xboxp.h>

//
// From the read side (xsndtrk.c): the ST.DB path and the read-cache lookasides
// the write path must invalidate after mutating the database.
//
extern const OBJECT_STRING XapiStDbFile;
extern STDB_LIST   XapiListLookaside;
extern STDB_STDESC XapiStLookaside;
extern BOOL XapiReadFromStDb(HANDLE Handle, PBYTE Buffer, DWORD Signature);

//
// Kept a separate translation unit so a title that uses only the read side never
// pulls this object (and its ASF/heap machinery) into the link.
//

#define STDB_MUSICDIR "\\Device\\Harddisk0\\partition1\\TDATA\\FFFE0000\\MUSIC\\"

#ifndef FILE_CREATED
#define FILE_CREATED 0x00000002     // NtCreateFile IoStatusBlock.Information
#endif

//
// Serializes all soundtrack-database mutation. The dashboard and other title
// threads share ST.DB, so writers hold this across the whole open/modify/close.
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
// Page seek helpers (reconcile with the read side's page math):
//   descriptor page for soundtrack slot i  -> (i + 1) * STDB_PAGE_SIZE
//   list-block page for list index n        -> (n + 1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE
//
static BOOL XapiSeekPage(HANDLE h, ULONG page)
{
    return SetFilePointer(h, page * STDB_PAGE_SIZE, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

static BOOL XapiWritePage(HANDLE h, PVOID Buffer)
{
    DWORD n;
    return WriteFile(h, Buffer, STDB_PAGE_SIZE, &n, NULL) && n == STDB_PAGE_SIZE;
}

//
// Open (creating if absent) the soundtrack database read/write and read its
// header. A freshly created database is a header page followed by MAX_SOUNDTRACKS
// zeroed descriptor pages (no list blocks yet) -- exactly what the read side
// expects. Also ensures the MUSIC directory exists. Caller holds the CS.
//
static NTSTATUS XapiBeginUsingSoundtracks(HANDLE *phFile, PSTDB_HDR pHeader)
{
    NTSTATUS          status;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK   iosb;
    ANSI_STRING       as;
    HANDLE            hDir, hFile;

    //
    // Ensure the MUSIC directory exists (tolerate "already there").
    //
    RtlInitAnsiString(&as, STDB_MUSICDIR);
    InitializeObjectAttributes(&obja, (POBJECT_STRING)&as, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtCreateFile(&hDir, GENERIC_READ | SYNCHRONIZE, &obja, &iosb, NULL,
                          FILE_ATTRIBUTE_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status))
        NtClose(hDir);
    else if (status != STATUS_OBJECT_NAME_COLLISION)
        return status;

    //
    // Open/create the database read/write.
    //
    InitializeObjectAttributes(&obja, (POBJECT_STRING)&XapiStDbFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtCreateFile(&hFile, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &obja, &iosb, NULL,
                          FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status))
        return status;

    if (iosb.Information == FILE_CREATED)
    {
        //
        // New database: lay down the header + MAX_SOUNDTRACKS empty descriptor
        // pages in one zeroed image, then rewind.
        //
        ULONG  cb = (1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE;
        PBYTE  p  = (PBYTE)XapiAlloc(cb);
        DWORD  written;

        if (p == NULL)
        {
            iosb.Information = 0;
            { BYTE del = TRUE; NtSetInformationFile(hFile, &iosb, &del, sizeof(del), FileDispositionInformation); }
            NtClose(hFile);
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory(p, cb);
        ((PSTDB_HDR)p)->Version = STDB_VERSION;

        if (!WriteFile(hFile, p, cb, &written, NULL) || written != cb)
        {
            XapiFree(p);
            { BYTE del = TRUE; NtSetInformationFile(hFile, &iosb, &del, sizeof(del), FileDispositionInformation); }
            NtClose(hFile);
            return STATUS_UNSUCCESSFUL;
        }
        XapiFree(p);
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    }

    //
    // Read the header page.
    //
    if (!XapiReadFromStDb(hFile, (PBYTE)pHeader, 0))
    {
        NtClose(hFile);
        return STATUS_UNSUCCESSFUL;
    }

    *phFile = hFile;
    return STATUS_SUCCESS;
}

static VOID XapiEndUsingSoundtracks(HANDLE hFile)
{
    //
    // All writes are write-through; nothing to flush. Invalidate the read caches
    // so a subsequent query re-reads the mutated database.
    //
    XapiListLookaside.StId = INVALID_STID;
    XapiStLookaside.Id     = INVALID_STID;
    if (hFile)
        NtClose(hFile);
}

//
// Persist the header page (page 0) from the in-memory copy.
//
static BOOL XapiWriteHeader(HANDLE h, PSTDB_HDR pHeader)
{
    BYTE page[STDB_PAGE_SIZE];
    RtlZeroMemory(page, sizeof(page));
    RtlCopyMemory(page, pHeader, sizeof(STDB_HDR));
    return XapiSeekPage(h, 0) && XapiWritePage(h, page);
}

//
// Allocate the next monotonic song ID for a soundtrack. The ID packs the
// soundtrack index in the high word and a per-database sequence in the low word
// -- exactly what the read side decodes as MUSIC\%04x\%08x.WMA. NextSongId is
// persisted immediately.
//
static UINT XapiSoundtrackNewSongID(HANDLE h, PSTDB_HDR pHeader, UINT SoundtrackId)
{
    UINT seq = pHeader->NextSongId;
    pHeader->NextSongId = seq + 1;

    if (!XapiWriteHeader(h, pHeader))
        return INVALID_STID;

    return (SoundtrackId << 16) | (seq & 0xFFFF);
}

//
// Find a reusable (InUse == 0) list block, or append a fresh one at end of file
// and return its index. The on-disk page for index n is (n + 1 + MAX_SOUNDTRACKS).
//
static UINT XapiSoundtrackFindNewListBlock(HANDLE h)
{
    BYTE page[STDB_PAGE_SIZE];
    UINT n;

    for (;;)
    {
        if (!XapiSeekPage(h, 1 + MAX_SOUNDTRACKS))
            return INVALID_STID;

        for (n = 0; ; n++)
        {
            PSTDB_LIST pList = (PSTDB_LIST)page;

            if (!XapiReadFromStDb(h, page, STDB_LISTSIG))
            {
                //
                // Past EOF or a bad signature: append a fresh empty block and
                // rescan so its index is returned.
                //
                RtlZeroMemory(page, sizeof(page));
                pList->Signature = STDB_LISTSIG;
                pList->InUse = FALSE;
                if (SetFilePointer(h, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER ||
                    !XapiWritePage(h, page))
                    return INVALID_STID;
                break;      // rescan from the top
            }

            if (!pList->InUse)
                return n;   // reuse this free block
        }
    }
}

//
// Copy the source file into the (already created, pre-sized) destination handle,
// reporting progress. Returns S_OK or an HRESULT.
//
static HRESULT XapiCopySong(HANDLE hSrc, HANDLE hDst, PCWSTR pszSoundtrack, PCWSTR pszSong,
                            LP_SOUNDTRACK_PROGRESS_ROUTINE lpRoutine, LPVOID Context)
{
    PBYTE           buf;
    DWORD           read;
    LARGE_INTEGER   total, done;
    BOOL            report = TRUE;

    if (!GetFileSizeEx(hSrc, &total))
        return HRESULT_FROM_WIN32(GetLastError());

    buf = (PBYTE)XapiAlloc(0x10000);
    if (buf == NULL)
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

    done.QuadPart = 0;

    if (lpRoutine)
    {
        DWORD r = lpRoutine(pszSoundtrack, pszSong, total, done, Context);
        if (r == 3) report = FALSE;
        else if (r != 0) { XapiFree(buf); return HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED); }
    }

    while (done.QuadPart < total.QuadPart)
    {
        DWORD written;
        if (!ReadFile(hSrc, buf, 0x10000, &read, NULL) || read == 0)
            break;
        if (!WriteFile(hDst, buf, read, &written, NULL) || written != read)
        {
            XapiFree(buf);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        done.QuadPart += read;

        if (report && lpRoutine)
        {
            DWORD r = lpRoutine(pszSoundtrack, pszSong, total, done, Context);
            if (r == 3) report = FALSE;
            else if (r != 0) { XapiFree(buf); return HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED); }
        }
    }

    XapiFree(buf);
    return S_OK;
}

//
// Parse the ASF "Play Duration" (100ns units) out of a WMA header and return it
// in milliseconds, or 0 if it cannot be found. Self-contained so libxapi needs
// no decoder. See the caveat in the file header: this is the one field not
// verified against retail.
//
static UINT XapiWmaDurationMs(HANDLE hSrc)
{
    // ASF File Properties Object GUID.
    static const BYTE kFileProps[16] = {
        0xA1,0xDC,0xAB,0x8C, 0x47,0xA9, 0xCF,0x11, 0x8E,0xE4, 0x00,0xC0,0x0C,0x20,0x53,0x65
    };
    BYTE  hdr[0x2000];
    DWORD read, i;
    LARGE_INTEGER zero = { 0 };

    SetFilePointerEx(hSrc, zero, NULL, FILE_BEGIN);
    if (!ReadFile(hSrc, hdr, sizeof(hdr), &read, NULL) || read < 0x60)
    {
        SetFilePointerEx(hSrc, zero, NULL, FILE_BEGIN);
        return 0;
    }
    SetFilePointerEx(hSrc, zero, NULL, FILE_BEGIN);

    //
    // Scan for the File Properties Object GUID; PlayDuration is a QWORD 64 bytes
    // into the object (GUID 16 + size 8 + fileid 16 + filesize 8 + created 8 +
    // packets 8), and Preroll (ms) is 16 bytes past it.
    //
    for (i = 0; i + 0x58 <= read; i++)
    {
        if (RtlEqualMemory(hdr + i, kFileProps, 16))
        {
            ULONGLONG playDuration = *(ULONGLONG UNALIGNED *)(hdr + i + 64);
            ULONGLONG preroll      = *(ULONGLONG UNALIGNED *)(hdr + i + 80);
            ULONGLONG ms = playDuration / 10000;
            if (ms > preroll) ms -= preroll;
            return (UINT)ms;
        }
    }
    return 0;
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
    // HIWORD of every song ID stored under it).
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
        s = NtCreateFile(&hDir, GENERIC_READ | SYNCHRONIZE, &obja, &iosb, NULL,
                         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
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
    // Write the descriptor page BEFORE the header, so the header's StCount/StBlocks
    // never point at a descriptor that was not written.
    //
    {
        BYTE page[STDB_PAGE_SIZE];
        RtlZeroMemory(page, sizeof(page));
        RtlCopyMemory(page, &desc, sizeof(desc));
        if (!XapiSeekPage(hFile, slot + 1) || !XapiWritePage(hFile, page))
        {
            XapiSetLastNTError(STATUS_UNSUCCESSFUL);
            rv = 0;
            goto Exit;
        }
    }

    if (!XapiWriteHeader(hFile, &header))
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
    UINT        i, slot, listIndex, listBlock, songId, songLen;
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
    for (i = 0; i < MAX_SOUNDTRACKS && i < header.StCount; i++)
        if (header.StBlocks[i] == dwSoundtrackId)
            break;
    if (i >= header.StCount || i >= MAX_SOUNDTRACKS)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    {
        BYTE page[STDB_PAGE_SIZE];
        if (!XapiSeekPage(hFile, i + 1) || !XapiReadFromStDb(hFile, page, STDB_STSIG))
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
    // Open the source WMA and derive its length + name.
    //
    hSrc = CreateFileA(pszSongPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hSrc == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    songLen = XapiWmaDurationMs(hSrc);

    if (pszSongName)
        pName = pszSongName;
    else
    {
        lstrcpynW(nameBuf, L"Unknown", MAX_SONG_NAME);
        pName = nameBuf;
    }

    //
    // Allocate a song ID and its slot within a list block.
    //
    songId = XapiSoundtrackNewSongID(hFile, &header, dwSoundtrackId);
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
    // Create the destination file, pre-sized to the source.
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

        status = NtCreateFile(&hDst, GENERIC_WRITE | SYNCHRONIZE, &obja, &iosb, &srcSize,
                              FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE,
                              FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
        if (!NT_SUCCESS(status))
        {
            hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));
            hDst = INVALID_HANDLE_VALUE;
            goto Exit;
        }
    }

    hr = XapiCopySong(hSrc, hDst, desc.Name, pName, lpRoutine, Context);
    if (FAILED(hr))
        goto Exit;

    //
    // Update descriptor: bump length + song count, allocate a list block on a new
    // group boundary.
    //
    desc.SoundtrackLength += songLen;
    desc.SongCount        += 1;

    if (slot == 0)
    {
        listBlock = XapiSoundtrackFindNewListBlock(hFile);
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
        if (!XapiSeekPage(hFile, i + 1) || !XapiWritePage(hFile, page))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
            goto Exit;
        }
    }

    //
    // Read the list block, insert the song, write it back.
    //
    if (!XapiSeekPage(hFile, listBlock + 1 + MAX_SOUNDTRACKS) ||
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

    if (!XapiSeekPage(hFile, listBlock + 1 + MAX_SOUNDTRACKS) || !XapiWritePage(hFile, listPage))
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
