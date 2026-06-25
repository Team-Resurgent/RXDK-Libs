/*++
    XInput light gun helpers — ported from reference lightgun.obj disassembly.
--*/

#define _XAPI_
extern "C" {
#include <ntos.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <xtl.h>
#include <xboxp.h>

#define MODULE_POOL_TAG '_ngl'
#include <debug.h>
DEFINE_USB_DEBUG_FUNCTIONS("LIGHTGUN");

#include <usb.h>
#include "../xid/xid.h"

static void __fastcall XID_fWaitForUrb(IUsbDevice *Device, PURB Urb, PKEVENT Event)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Urb);
    KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
}

static BOOL LightgunIsValidOpenDevice(PXID_OPEN_DEVICE OpenDevice)
{
    if (!OpenDevice || !OpenDevice->XidNode || OpenDevice->XidNode->PendingRemove)
        return FALSE;

    if (!OpenDevice->TypeInformation ||
        OpenDevice->TypeInformation->ucType != XID_DEVTYPE_GAMECONTROLLER)
        return FALSE;

    if (OpenDevice->XidNode->SubType != XINPUT_DEVSUBTYPE_GC_LIGHTGUN)
        return FALSE;

    return TRUE;
}

extern "C" DWORD WINAPI XInputGetDeviceDescription(
    HANDLE hDevice,
    PXINPUT_DEVICE_DESCRIPTION pDescription)
{
    PXID_OPEN_DEVICE openDevice = (PXID_OPEN_DEVICE)hDevice;
    PXID_DEVICE_NODE xidNode;
    IUsbDevice *device;
    KEVENT event;
    URB urb;
    USB_DEVICE_DESCRIPTOR deviceDescriptor;
    KIRQL savedIrql;
    DWORD errorCode;

    if (!pDescription)
        return ERROR_INVALID_PARAMETER;

    savedIrql = KeRaiseIrqlToDpcLevel();

    if (!openDevice || !openDevice->XidNode || openDevice->XidNode->PendingRemove)
    {
        KeLowerIrql(savedIrql);
        return ERROR_DEVICE_NOT_CONNECTED;
    }

    xidNode = openDevice->XidNode;
    device = xidNode->Device;

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    RtlZeroMemory(&urb, sizeof(urb));
    RtlZeroMemory(&deviceDescriptor, sizeof(deviceDescriptor));

    USB_BUILD_CONTROL_TRANSFER(
        &urb.ControlTransfer,
        NULL,
        &deviceDescriptor,
        sizeof(USB_DEVICE_DESCRIPTOR),
        USB_TRANSFER_DIRECTION_IN,
        (PURB_COMPLETE_PROC)XID_SyncComplete,
        &event,
        TRUE,
        (USB_DEVICE_TO_HOST | USB_STANDARD_COMMAND | USB_COMMAND_TO_DEVICE),
        USB_REQUEST_GET_DESCRIPTOR,
        (USB_DEVICE_DESCRIPTOR_TYPE << 8),
        0,
        sizeof(USB_DEVICE_DESCRIPTOR));

    device->SubmitRequest(&urb);
    KeLowerIrql(savedIrql);
    XID_fWaitForUrb(device, &urb, &event);
    KeRaiseIrqlToDpcLevel();

    if (!USBD_SUCCESS(urb.Header.Status))
    {
        errorCode = IUsbDevice::Win32FromUsbdStatus(urb.Header.Status);
    }
    else if (urb.ControlTransfer.TransferBufferLength < 0x0E)
    {
        errorCode = ERROR_GEN_FAILURE;
    }
    else
    {
        pDescription->wVendorID = deviceDescriptor.idVendor;
        pDescription->wProductID = deviceDescriptor.idProduct;
        pDescription->wVersion = deviceDescriptor.bcdDevice;
        errorCode = ERROR_SUCCESS;
    }

    KeLowerIrql(savedIrql);
    return errorCode;
}

extern "C" DWORD WINAPI XInputSetLightgunCalibration(
    HANDLE hDevice,
    PXINPUT_LIGHTGUN_CALIBRATION_OFFSETS pCalibrationOffsets)
{
    PXID_OPEN_DEVICE openDevice = (PXID_OPEN_DEVICE)hDevice;
    BYTE feedbackBuffer[sizeof(XINPUT_FEEDBACK_INTERNAL) + 8];
    PXINPUT_FEEDBACK_INTERNAL feedback = (PXINPUT_FEEDBACK_INTERNAL)feedbackBuffer;
    PBYTE reportBytes;
    HANDLE completionEvent;
    DWORD errorCode;
    KIRQL savedIrql;

    if (!pCalibrationOffsets)
        return ERROR_INVALID_PARAMETER;

    savedIrql = KeRaiseIrqlToDpcLevel();

    if (!LightgunIsValidOpenDevice(openDevice))
    {
        KeLowerIrql(savedIrql);
        return openDevice ? ERROR_INVALID_PARAMETER : ERROR_DEVICE_NOT_CONNECTED;
    }

    KeLowerIrql(savedIrql);

    RtlZeroMemory(feedbackBuffer, sizeof(feedbackBuffer));
    feedback->dwStatus = ERROR_IO_PENDING;

    completionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!completionEvent)
        return GetLastError();

    feedback->hEvent = completionEvent;
    feedback->Internal.bReportId = 1;
    feedback->Internal.bSize = 10;

    reportBytes = (PBYTE)&feedback->Internal.bReportId;
    reportBytes[0] = 1;
    reportBytes[1] = 10;
    RtlCopyMemory(reportBytes + 2, pCalibrationOffsets, sizeof(*pCalibrationOffsets));

    errorCode = XID_fSendDeviceReport(openDevice, feedback);
    if (errorCode != ERROR_IO_PENDING)
    {
        CloseHandle(completionEvent);
        return errorCode;
    }

    WaitForSingleObject(completionEvent, INFINITE);
    CloseHandle(completionEvent);
    return feedback->dwStatus;
}
