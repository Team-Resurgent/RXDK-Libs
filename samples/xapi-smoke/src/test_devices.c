#include "common.h"

int test_devices(void)
{
    xapi_smoke_trace_line("devices XInitDevices enter");
    XInitDevices(0, NULL);
    xapi_smoke_trace_line("devices XInitDevices done");

    (void)XGetDevices(XDEVICE_TYPE_GAMEPAD);

    DWORD insertions = 0;
    DWORD removals = 0;
    /* FALSE means no hotplug since the last XGetDevices/XGetDeviceChanges — not failure. */
    (void)XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);

    return XAPI_OK;
}
