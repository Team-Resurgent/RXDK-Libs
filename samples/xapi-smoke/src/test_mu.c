#include "common.h"

int test_mu(void)
{
    char drive[4];

    xapi_smoke_trace_line("mu enter");

    const DWORD err = XMountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT, drive);
    xapi_smoke_trace_count("mu XMountMU err", err);

    if (err == ERROR_DEVICE_NOT_CONNECTED) {
        return XAPI_OK;
    }
    if (err != ERROR_SUCCESS) {
        return XAPI_SKIP;
    }

    if (drive[0] == '\0' || drive[1] != ':') {
        (void)XUnmountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT);
        return 1;
    }

    if (GetFileAttributesA(drive) == INVALID_FILE_ATTRIBUTES) {
        (void)XUnmountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT);
        return 2;
    }

    (void)XUnmountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT);
    return XAPI_OK;
}
