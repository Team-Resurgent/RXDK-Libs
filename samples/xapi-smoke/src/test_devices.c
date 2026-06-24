#include "common.h"

int test_devices(void)
{
    XInitDevices(0, NULL);

    (void)XGetDevices(XDEVICE_TYPE_GAMEPAD);

    DWORD insertions = 0;
    DWORD removals = 0;
    /* FALSE means no hotplug since the last XGetDevices/XGetDeviceChanges — not failure. */
    (void)XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);

    return XAPI_OK;
}
