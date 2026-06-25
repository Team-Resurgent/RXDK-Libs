#include "common.h"

/* Write a small file to the mounted MU drive and read it back. */
static int mu_save_roundtrip(const char* drive)
{
    char path[16];
    const char payload[] = "rxdk-mu-smoke";
    char readbuf[sizeof(payload)] = { 0 };
    DWORD n = 0;
    HANDLE h;
    int i;

    lstrcpynA(path, drive, (int)sizeof(path));
    lstrcatA(path, "\\smk.sav");

    h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        xapi_smoke_trace_count("mu save create-w err", GetLastError());
        return 10;
    }
    if (!WriteFile(h, payload, sizeof(payload), &n, NULL) || n != sizeof(payload)) {
        CloseHandle(h);
        return 11;
    }
    CloseHandle(h);

    h = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        xapi_smoke_trace_count("mu save create-r err", GetLastError());
        return 12;
    }
    if (!ReadFile(h, readbuf, sizeof(payload), &n, NULL) || n != sizeof(payload)) {
        CloseHandle(h);
        return 13;
    }
    CloseHandle(h);

    for (i = 0; i < (int)sizeof(payload); ++i) {
        if (readbuf[i] != payload[i]) {
            return 14;
        }
    }

    DeleteFileA(path);
    return XAPI_OK;
}

int test_mu(void)
{
    char drive[4];
    DWORD mus;
    DWORD port = 0;
    DWORD slot = 0;
    int found = 0;
    DWORD bit;
    int i;
    DWORD err;
    int rc;

    xapi_smoke_trace_line("mu enter");

    /* MUs connect asynchronously after XInitDevices; poll until one appears
     * (or give up after ~1s) so we don't race the USB enumeration. */
    mus = 0;
    for (i = 0; i < 50; ++i) {
        mus = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);
        if (mus != 0) {
            break;
        }
        Sleep(20);
    }
    xapi_smoke_trace_count("mu XGetDevices bitmask", mus);
    if (mus == 0) {
        xapi_smoke_trace_line("mu no MU connected");
        return XAPI_OK;
    }

    /* Bit index == node index == (port << 1) | slot. Pick the first present MU. */
    for (bit = 0; bit < 8; ++bit) {
        if (mus & (1u << bit)) {
            port = bit >> 1;
            slot = bit & 1u;
            found = 1;
            break;
        }
    }
    if (!found) {
        return 1;
    }
    xapi_smoke_trace_count("mu port", port);
    xapi_smoke_trace_count("mu slot", slot);

    /* XMountMU writes a single drive LETTER to *pchDrive (not a "X:" string). */
    drive[0] = 0;
    err = XMountMU(port, slot, drive);
    xapi_smoke_trace_count("mu XMountMU err", err);
    if (err != ERROR_SUCCESS) {
        /* Unformatted/raw or not connected: not a hard failure for the smoke. */
        return XAPI_SKIP;
    }

    if (drive[0] < 'A' || drive[0] > 'Z') {
        XUnmountMU(port, slot);
        return 2;
    }
    /* Build a "<letter>:" root from the returned drive letter. */
    drive[1] = ':';
    drive[2] = '\0';
    xapi_smoke_trace_line2("mu drive ", drive);

    rc = mu_save_roundtrip(drive);
    xapi_smoke_trace_count("mu save rc", (unsigned)rc);

    XUnmountMU(port, slot);
    return rc;
}
