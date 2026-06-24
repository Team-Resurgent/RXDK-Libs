#include "common.h"

int test_xinput(void)
{
    XInitDevices(0, NULL);

    HANDLE device = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT0, XDEVICE_NO_SLOT, NULL);
    if (!device) {
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
