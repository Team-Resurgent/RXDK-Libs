#include "common.h"

int test_mu(void)
{
    char drive[4];

    DWORD err = XMountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT, drive);
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
