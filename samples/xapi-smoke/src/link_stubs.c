/* Link stubs for xapi-smoke until full USB/xid stack links cleanly. */

#include "common.h"

XPP_DEVICE_TYPE XDEVICE_TYPE_GAMEPAD_TABLE;

HANDLE WINAPI XInputOpen(
    PXPP_DEVICE_TYPE DeviceType,
    DWORD dwPort,
    DWORD dwSlot,
    PXINPUT_POLLING_PARAMETERS pPollingParameters)
{
    (void)DeviceType;
    (void)dwPort;
    (void)dwSlot;
    (void)pPollingParameters;
    return NULL;
}

DWORD WINAPI XInputGetCapabilities(HANDLE hDevice, PXINPUT_CAPABILITIES pCapabilities)
{
    (void)hDevice;
    (void)pCapabilities;
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetState(HANDLE hDevice, PXINPUT_STATE pState)
{
    (void)hDevice;
    (void)pState;
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputPoll(HANDLE hDevice)
{
    (void)hDevice;
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(HANDLE hDevice, PXINPUT_FEEDBACK pFeedback)
{
    (void)hDevice;
    (void)pFeedback;
    return ERROR_DEVICE_NOT_CONNECTED;
}

VOID WINAPI XInputClose(HANDLE hDevice)
{
    (void)hDevice;
}

VOID USBD_Init(void)
{
}

NTSTATUS MU_CreateDeviceObject(ULONG Port, ULONG Slot, POBJECT_STRING DeviceName)
{
    (void)Port;
    (void)Slot;
    (void)DeviceName;
    return (NTSTATUS)0xC0000002L;
}

VOID MU_CloseDeviceObject(ULONG Port, ULONG Slot)
{
    (void)Port;
    (void)Slot;
}

PDEVICE_OBJECT MU_GetExistingDeviceObject(ULONG Port, ULONG Slot)
{
    (void)Port;
    (void)Slot;
    return NULL;
}
