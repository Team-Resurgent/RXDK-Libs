#include "common.h"

static DWORD xapi_smoke_last_xinput_open_error;

static HANDLE xapi_smoke_try_open_gamepad_on_port(DWORD port)
{
    HANDLE device = XInputOpen(XDEVICE_TYPE_GAMEPAD, port, XDEVICE_NO_SLOT, NULL);
    if (!device) {
        xapi_smoke_last_xinput_open_error = GetLastError();
    }
    return device;
}

static HANDLE xapi_smoke_open_gamepad(void)
{
    const DWORD port_count = XGetPortCount();
    DWORD port;

    (void)XGetDevices(XDEVICE_TYPE_GAMEPAD);

    for (port = 0; port < port_count; ++port) {
        HANDLE device = xapi_smoke_try_open_gamepad_on_port(port);
        if (device) {
            return device;
        }
    }
    return NULL;
}

static HANDLE xapi_smoke_wait_for_gamepad(DWORD timeout_ms)
{
    const DWORD start = GetTickCount();
    HANDLE device;

    for (;;) {
        DWORD insertions = 0;
        DWORD removals = 0;

        (void)XGetDevices(XDEVICE_TYPE_GAMEPAD);
        (void)XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);

        device = xapi_smoke_open_gamepad();
        if (device) {
            return device;
        }

        if ((GetTickCount() - start) >= timeout_ms) {
            return NULL;
        }
        Sleep(50);
    }
}

int test_xinput(void)
{
    HANDLE device = xapi_smoke_wait_for_gamepad(5000);
    if (!device) {
        const DWORD gamepad_mask = XGetDevices(XDEVICE_TYPE_GAMEPAD);
        xapi_smoke_trace_count("xinput gamepad mask", gamepad_mask);
        if (xapi_smoke_last_xinput_open_error == 87u) {
            xapi_smoke_trace_line("xinput err 87 invalid type (XID type table)");
        } else if (xapi_smoke_last_xinput_open_error == 116u) {
            xapi_smoke_trace_line("xinput err 116 no gamepad on port");
        }
        xapi_smoke_trace_fail("xinput open", xapi_smoke_last_xinput_open_error);
        return XAPI_SKIP;
    }

    XINPUT_CAPABILITIES caps;
    const DWORD capErr = XInputGetCapabilities(device, &caps);
    if (capErr != ERROR_SUCCESS) {
        XInputClose(device);
        return 4;
    }

    XINPUT_STATE state;
    const DWORD err = XInputGetState(device, &state);
    if (err != ERROR_SUCCESS) {
        XInputClose(device);
        return 5;
    }

    if (XInputPoll(device) != ERROR_SUCCESS) {
        XInputClose(device);
        return 6;
    }

    XINPUT_FEEDBACK feedback;
    feedback.Rumble.wLeftMotorSpeed = 0;
    feedback.Rumble.wRightMotorSpeed = 0;
    (void)XInputSetState(device, &feedback);

    XInputClose(device);
    return XAPI_OK;
}
