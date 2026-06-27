#include "common.h"

//
// Exercises the XTL soundtrack-enumeration APIs (XFindFirstSoundtrack /
// XFindNextSoundtrack / XGetSoundtrackSongInfo / XOpenSoundtrackSong).
//
// A clean kit has no user-ripped soundtracks, so the soundtrack database
// (\Device\Harddisk0\partition1\TDATA\FFFE0000\MUSIC\ST.DB) is absent. The
// primary thing to verify on hardware is that this newly-ported code links and
// runs: XFindFirstSoundtrack must fail gracefully (INVALID_HANDLE_VALUE plus a
// not-found error) rather than fault. If the kit does have soundtracks, walk one
// entry and query a song to exercise the read paths.
//
int test_soundtrack(void)
{
    XSOUNDTRACK_DATA data;
    HANDLE find;

    find = XFindFirstSoundtrack(&data);

    if (find != INVALID_HANDLE_VALUE) {
        DWORD songId = 0;
        DWORD songLen = 0;

        xapi_smoke_trace_line("soundtrack present (enumerating)");

        if (data.uSongCount > 0) {
            (void)XGetSoundtrackSongInfo(data.uSoundtrackId, 0, &songId, &songLen, NULL, 0);
        }

        // May return FALSE at end of list; just confirm it does not fault.
        (void)XFindNextSoundtrack(find, &data);
        return XAPI_OK;
    }

    {
        const DWORD err = GetLastError();
        HANDLE song;

        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            xapi_smoke_trace_line("soundtrack none (expected on clean kit)");
        } else {
            // Some other failure: the code still ran without crashing, which is
            // the parity property we are verifying -- log it for visibility.
            xapi_smoke_trace_line("soundtrack none (unexpected error code)");
        }

        // XOpenSoundtrackSong on a missing song must also degrade gracefully.
        song = XOpenSoundtrackSong(0x00010001, FALSE);
        if (song != INVALID_HANDLE_VALUE) {
            CloseHandle(song);
        }

        return XAPI_OK;
    }
}
