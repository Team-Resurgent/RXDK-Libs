/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    xsndtrk.c

Abstract:

    XTL Soundtrack Enumeration APIs

    Provides support for enumerating the soundtracks a user has ripped through
    the dash and enumerating the songs within those soundtracks. Ported from the
    original xapilib source; reads the on-disk soundtrack database
    (TDATA\FFFE0000\MUSIC\ST.DB) described by the STDB_* structures in xboxp.h.

--*/

#include "bridge_k32.h"
#include "basedll.h"
#include <xboxp.h>

const OBJECT_STRING XapiStDbFile = CONSTANT_OBJECT_STRING(OTEXT("\\Device\\Harddisk0\\partition1\\TDATA\\FFFE0000\\MUSIC\\ST.DB"));
#define STDB_MUSICDIR "\\Device\\Harddisk0\\partition1\\TDATA\\FFFE0000\\MUSIC\\"

//
// Lookaside variables for fast access to data when querying for songs.
//
STDB_LIST XapiListLookaside = {0, INVALID_STID};
STDB_STDESC XapiStLookaside  = {0, INVALID_STID};

BOOL
XapiReadFromStDb(
    HANDLE Handle,
    PBYTE Buffer,
    DWORD Signature
    )

/*++

Routine Description:

    Simple helper routine for reading a page from the database.

Arguments:

    Handle - Valid file handle of the open soundtrack database.
    Buffer - Buffer for the read (at least STDB_PAGE_SIZE bytes).
    Signature - If non-zero, the first DWORD of the buffer is checked against it.

Return Value:

    TRUE if the data was read and the (optional) signature matches.

--*/

{
    DWORD byteCount;
    return ReadFile(Handle, Buffer, STDB_PAGE_SIZE, &byteCount, NULL)
        && byteCount == STDB_PAGE_SIZE
        && (!Signature || (*(PDWORD)Buffer) == Signature);
}

NTSTATUS
XapiOpenStDbAndReadHeader(
    HANDLE *Handle,
    PBYTE Buffer
    )

/*++

Routine Description:

    Opens the soundtrack database and validates the header.

Arguments:

    Handle - Receives a valid file handle on success.
    Buffer - Read buffer; receives the database header on success.

Return Value:

    STATUS_SUCCESS or an error code.

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;

    InitializeObjectAttributes(&obja, (POBJECT_STRING)&XapiStDbFile, OBJ_CASE_INSENSITIVE, NULL, NULL);

    //
    // Attempt to open the soundtrack database.
    //
    status = NtCreateFile(
                Handle,
                SYNCHRONIZE | GENERIC_READ,
                &obja,
                &iosb,
                NULL,
                FILE_ATTRIBUTE_SYSTEM | FILE_NO_INTERMEDIATE_BUFFERING,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT
                );

    if (NT_SUCCESS(status) && !XapiReadFromStDb(*Handle, Buffer, 0)) {
        status = STATUS_UNSUCCESSFUL;
        NtClose(*Handle);
    }

    return status;
}

BOOL
XapiGetNextSoundtrack(
    PBYTE Buffer,
    PSNDTRK_ENUMSTATE State,
    PXSOUNDTRACK_DATA Data
    )

/*++

Routine Description:

    Worker function for retrieving the next soundtrack from the database.

Arguments:

    Buffer - Temporary read buffer (contents are not preserved across the call).
    State - Internal enumeration state.
    Data - Receives the soundtrack data for the next soundtrack.

Return Value:

    TRUE if there are additional soundtracks to enumerate, FALSE otherwise.

--*/

{
    BOOL ok = FALSE;

    if (State->Index < State->MaxIndex) {
        SetFilePointer(State->DbHandle, STDB_PAGE_SIZE * (State->Index + 1), NULL, FILE_BEGIN);
        State->Index++;
        ok = XapiReadFromStDb(State->DbHandle, Buffer, STDB_STSIG);
    }

    if (ok) {
        memcpy(&XapiStLookaside, Buffer, sizeof(STDB_STDESC));
        wcsncpy(Data->szName, XapiStLookaside.Name, MAX_SOUNDTRACK_NAME);
        Data->uSoundtrackId     = XapiStLookaside.Id;
        Data->uSongCount        = XapiStLookaside.SongCount;
        Data->uSoundtrackLength = XapiStLookaside.SoundtrackLength;
    }

    return ok;
}

BOOL
__attribute__((__stdcall__))
XFindNextSoundtrack(
    IN HANDLE FindHandle,
    OUT PXSOUNDTRACK_DATA SoundtrackData
    )

/*++

Routine Description:

    Continues an enumeration of soundtracks begun with XFindFirstSoundtrack.

Arguments:

    FindHandle - A handle returned from XFindFirstSoundtrack.
    SoundtrackData - On success, receives the next soundtrack in the list.

Return Value:

    TRUE if there are additional soundtracks to enumerate, FALSE otherwise.

--*/

{
    BYTE buf[STDB_PAGE_SIZE];

    RIP_ON_NOT_TRUE("XFindNextSoundtrack()", (NULL != FindHandle));
    RIP_ON_NOT_TRUE("XFindNextSoundtrack()", (INVALID_HANDLE_VALUE != FindHandle));
    RIP_ON_NOT_TRUE("XFindNextSoundtrack()", (NULL != SoundtrackData));

#if DBG
    if (FH_SIG_SOUNDTRACK != ((PSNDTRK_ENUMSTATE)FindHandle)->Signature) {
        RIP("XFindNextSoundtrack() - invalid parameter (FindHandle)");
    }
#endif // DBG

    return XapiGetNextSoundtrack(buf, (PSNDTRK_ENUMSTATE)FindHandle, SoundtrackData);
}

HANDLE
__attribute__((__stdcall__))
XFindFirstSoundtrack(
    OUT PXSOUNDTRACK_DATA SoundtrackData
    )

/*++

Routine Description:

    Begins an enumeration of the soundtracks on the media and returns the
    first soundtrack.

Arguments:

    SoundtrackData - On success, receives the data of the first soundtrack.

Return Value:

    A valid HANDLE for use with XFindNextSoundtrack on success, otherwise
    INVALID_HANDLE_VALUE.

--*/

{
    HANDLE h;
    PSNDTRK_ENUMSTATE state = NULL;
    NTSTATUS status;
    BYTE buf[STDB_PAGE_SIZE];
    PSTDB_HDR hdr;

    RIP_ON_NOT_TRUE("XFindFirstSoundtrack()", (NULL != SoundtrackData));

    status = XapiOpenStDbAndReadHeader(&h, buf);

    //
    // Build the enum state data that will be passed back to the caller.
    //
    if (NT_SUCCESS(status)) {
        hdr   = (PSTDB_HDR)buf;
        state = LocalAlloc(LMEM_FIXED, sizeof(SNDTRK_ENUMSTATE));

        if (!state) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            SetLastError(RtlNtStatusToDosError(status));
            NtClose(h);
        } else {
            state->Signature = FH_SIG_SOUNDTRACK;
            state->MaxIndex  = hdr->StCount;
            state->Index     = 0;
            state->DbHandle  = h;
        }
    } else {
        SetLastError(RtlNtStatusToDosError(status));
    }

    //
    // Now, attempt to retrieve the first soundtrack from the database.
    //
    if (NT_SUCCESS(status)) {
        if (!XapiGetNextSoundtrack(buf, state, SoundtrackData)) {
            NtClose(h);
            LocalFree(state);
            state = NULL;
        }
    }

    return state ? (HANDLE)state : INVALID_HANDLE_VALUE;
}

BOOL
__attribute__((__stdcall__))
XGetSoundtrackSongInfo(
    IN DWORD SoundtrackId,
    IN UINT Index,
    OUT PDWORD SongId,
    OUT PDWORD SongLength,
    OUT OPTIONAL PWSTR NameBuffer,
    IN UINT BufferSize
    )

/*++

Routine Description:

    Retrieves information about a particular song index in a soundtrack.

Arguments:

    SoundtrackId - The soundtrack identifier of interest.
    Index - The index of the song within the soundtrack.
    SongId - Receives the unique identifier of the song at that index.
    SongLength - Receives the song length.
    NameBuffer - Optionally receives the UNICODE name of the song.
    BufferSize - Size of NameBuffer in UNICODE characters (if non-NULL).

Return Value:

    TRUE if the data was successfully returned, FALSE otherwise.

Notes:

    Songs are batched in groups of WMADESC_LIST_SIZE within the database, so as
    long as the index stays within the same group as the previous call no
    additional read is necessary (the lookaside variables cache the last block).

--*/

{
    BOOL ok = TRUE;
    HANDLE h = INVALID_HANDLE_VALUE;
    PSTDB_HDR hdr;
    BYTE buf[STDB_PAGE_SIZE];
    UINT i;
    UINT val;
    NTSTATUS status;

    RIP_ON_NOT_TRUE("XGetSongInfo()", (INVALID_STID != SoundtrackId));
    RIP_ON_NOT_TRUE("XGetSongInfo()", (NULL != SongId));

#if DBG
    if (NameBuffer && BufferSize < MAX_SONG_NAME) {
        RIP("XGetSongInfo() - 'NameBuffer' Buffer too small.");
    }
#endif

    //
    // Make sure the correct soundtrack is in the lookaside list.
    //
    if (XapiStLookaside.Id != SoundtrackId) {
        status = XapiOpenStDbAndReadHeader(&h, buf);
        ok     = NT_SUCCESS(status);
        if (ok) {
            //
            // The header contains an array of all the possible soundtrack blocks
            // and the soundtrack ids in each of those blocks. Read through the
            // array to find the block to seek to.
            //
            hdr = (PSTDB_HDR)buf;
            val = min(hdr->StCount, MAX_SOUNDTRACKS);

            for (i = 0; i < val; i++) {
                if (hdr->StBlocks[i] == SoundtrackId) {
                    break;
                }
            }

            if (i == val) {
                ok = FALSE;
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        } else {
            SetLastError(RtlNtStatusToDosError(status));
        }

        if (ok) {
            ok = (SetFilePointer(h, i * STDB_PAGE_SIZE, NULL, FILE_CURRENT) != INVALID_SET_FILE_POINTER);
            if (!ok) {
                SetLastError(ERROR_INVALID_DATA);
            }
        }

        if (ok) {
            ok = XapiReadFromStDb(h, buf, STDB_STSIG);
        }

        if (ok) {
            memcpy(&XapiStLookaside, buf, sizeof(STDB_STDESC));
            ok = XapiStLookaside.Id == SoundtrackId && XapiStLookaside.SongCount > Index;
            if (!ok) {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
    }

    //
    // Make sure the correct list segment is in the lookaside list.
    //
    if (ok) {
        if (XapiListLookaside.StId != SoundtrackId ||
            Index < (XapiListLookaside.ListIndex * WMADESC_LIST_SIZE) ||
            Index >= (XapiListLookaside.ListIndex + 1) * WMADESC_LIST_SIZE) {

            if (h == INVALID_HANDLE_VALUE) {
                status = XapiOpenStDbAndReadHeader(&h, buf);
                ok     = NT_SUCCESS(status);
                if (!ok) {
                    SetLastError(RtlNtStatusToDosError(status));
                }
            }

            if (ok) {
                val = (XapiStLookaside.ListBlocks[Index / WMADESC_LIST_SIZE] + 1 + MAX_SOUNDTRACKS) * STDB_PAGE_SIZE;
                ok  = (SetFilePointer(h, val, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
                if (!ok) {
                    SetLastError(ERROR_INVALID_DATA);
                }
            }

            if (ok) {
                ok = XapiReadFromStDb(h, buf, STDB_LISTSIG);
            }

            if (ok) {
                memcpy(&XapiListLookaside, buf, sizeof(STDB_LIST));
                ok = XapiListLookaside.StId == SoundtrackId;
                if (!ok) {
                    SetLastError(ERROR_INVALID_DATA);
                }
            }
        }
    }

    //
    // Get the song info for the selected song.
    //
    if (Index > XapiStLookaside.SongCount) {
        ok = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (ok) {
        *SongId     = XapiListLookaside.SongIds[Index % WMADESC_LIST_SIZE];
        *SongLength = XapiListLookaside.SongLengths[Index % WMADESC_LIST_SIZE];
        if (NameBuffer) {
            wcsncpy(NameBuffer, XapiListLookaside.SongNames[Index % WMADESC_LIST_SIZE], MAX_SONG_NAME);
        }
    }

    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }

    return ok;
}

HANDLE
__attribute__((__stdcall__))
XOpenSoundtrackSong(
    IN DWORD SongId,
    IN BOOL Asynchronous
    )

/*++

Routine Description:

    Opens the WMA song matching the song ID in the Dash Music directory.

Arguments:

    SongId - The song id to open.
    Asynchronous - TRUE to open for asynchronous (unbuffered) reads, FALSE for
                   synchronous reading.

Return Value:

    A valid file handle to the WMA song if successful, otherwise
    INVALID_HANDLE_VALUE.

--*/

{
    CHAR path[MAX_PATH];
    HANDLE h;
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;
    OBJECT_STRING oPath;

    sprintf(path, "%s%04x\\%08x.WMA", STDB_MUSICDIR, HIWORD(SongId), SongId);

    RtlInitObjectString(&oPath, path);
    InitializeObjectAttributes(&obja, &oPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    //
    // Attempt to open the soundtrack.
    //
    status = NtCreateFile(
                &h,
                SYNCHRONIZE | GENERIC_READ | FILE_READ_ATTRIBUTES,
                &obja,
                &iosb,
                NULL,
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | (Asynchronous ? FILE_NO_INTERMEDIATE_BUFFERING : FILE_SYNCHRONOUS_IO_NONALERT)
                );

    if (!NT_SUCCESS(status)) {
        XapiSetLastNTError(status);
    }

    return NT_SUCCESS(status) ? h : INVALID_HANDLE_VALUE;
}
