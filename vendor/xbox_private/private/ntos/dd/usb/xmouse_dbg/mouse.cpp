/*++
    Debug USB mouse (xmouse_dbg) — ported from reference mouse.obj disassembly.
--*/

#define _XAPI_
extern "C" {
#include <ntos.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <xtl.h>
#include <xboxp.h>

#define MODULE_POOL_TAG '_uom'
#include <debug.h>
DEFINE_USB_DEBUG_FUNCTIONS("XMOUSE");

#include <usb.h>
#include "../xid/xid.h"

static char __stdcall AddSignedByteWithSaturation(char a, char b)
{
    int sum = (int)a + (int)b;
    if (sum < -128)
        return (char)-128;
    if (sum > 127)
        return 127;
    return (char)sum;
}

static void __fastcall XID_fWaitForUrb(IUsbDevice *Device, PURB Urb, PKEVENT Event)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Urb);
    KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
}

#ifdef DEBUG_MOUSE

int __fastcall XID_MouseOpen(PXID_OPEN_DEVICE OpenDevice)
{
    PXID_DEVICE_NODE xidNode;
    IUsbDevice *device;
    KEVENT event;
    KIRQL oldIrql;

    if (!OpenDevice || !OpenDevice->XidNode)
        return 0;

    xidNode = OpenDevice->XidNode;
    device = xidNode->Device;

    OpenDevice->Urb.CommonTransfer.TransferBufferLength = 0;

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    USB_BUILD_CONTROL_TRANSFER(
        &OpenDevice->Urb.ControlTransfer,
        NULL,
        OpenDevice->ReportForUrb,
        0x20,
        USB_TRANSFER_DIRECTION_IN,
        (PURB_COMPLETE_PROC)XID_SyncComplete,
        &event,
        TRUE,
        (USB_DEVICE_TO_HOST | USB_CLASS_COMMAND | USB_COMMAND_TO_INTERFACE),
        XID_COMMAND_GET_REPORT,
        0x0100,
        xidNode->InterfaceNumber,
        0x20);

    oldIrql = KeRaiseIrqlToDpcLevel();
    device->SubmitRequest(&OpenDevice->Urb);
    KeLowerIrql(oldIrql);
    XID_fWaitForUrb(device, &OpenDevice->Urb, &event);
    KeRaiseIrqlToDpcLevel();

    if (!OpenDevice->XidNode || xidNode->PendingRemove)
        return 0;

    //
    // Boot mice often return 3–4 bytes (btn, dx, dy [+ report id]) or stall
    // GET_REPORT; only large reports select absolute (non-boot) parsing.
    //
    if (USBD_SUCCESS(OpenDevice->Urb.Header.Status)
        && OpenDevice->Urb.CommonTransfer.TransferBufferLength > 4)
    {
        OpenDevice->MouseRelativeMode = FALSE;
    }
    else
    {
        OpenDevice->MouseRelativeMode = TRUE;
    }

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    OpenDevice->Urb.CommonTransfer.TransferBufferLength = 0;
    OpenDevice->Urb.ControlTransfer.TransferBuffer = NULL;

    USB_BUILD_CONTROL_TRANSFER(
        &OpenDevice->Urb.ControlTransfer,
        NULL,
        NULL,
        0,
        0,
        (PURB_COMPLETE_PROC)XID_SyncComplete,
        &event,
        TRUE,
        USB_HOST_TO_DEVICE | USB_CLASS_COMMAND | USB_COMMAND_TO_INTERFACE,
        HID_SET_IDLE,
        HID_IDLE_INFINITE,
        xidNode->InterfaceNumber,
        0);

    device->SubmitRequest(&OpenDevice->Urb);
    KeLowerIrql(PASSIVE_LEVEL);
    XID_fWaitForUrb(device, &OpenDevice->Urb, &event);
    KeRaiseIrqlToDpcLevel();

    if (!OpenDevice->XidNode || xidNode->PendingRemove)
        return 0;

    return 1;
}

void FASTCALL XID_MouseNewData(PXID_OPEN_DEVICE OpenDevice)
{
    PXINPUT_MOUSE pMouse = (PXINPUT_MOUSE)OpenDevice->Report;
    ULONG cbTransfer = OpenDevice->Urb.CommonTransfer.TransferBufferLength;

    if (OpenDevice->MouseRelativeMode)
    {
        pMouse->cMickeysX = AddSignedByteWithSaturation(
            pMouse->cMickeysX,
            (char)OpenDevice->ReportForUrb[1]);
        pMouse->cMickeysY = AddSignedByteWithSaturation(
            pMouse->cMickeysY,
            (char)OpenDevice->ReportForUrb[2]);
        pMouse->cWheel = AddSignedByteWithSaturation(
            pMouse->cWheel,
            (char)OpenDevice->ReportForUrb[3]);
        pMouse->bButtons = OpenDevice->ReportForUrb[0];
    }
    else
    {
        if (cbTransfer > sizeof(OpenDevice->Report))
            cbTransfer = sizeof(OpenDevice->Report);

        RtlCopyMemory(pMouse, OpenDevice->ReportForUrb, cbTransfer);
    }

    XAutoPowerDownResetTimer();
}

VOID
XID_EnumMouse(
    PURB Urb,
    PXID_DEVICE_NODE XidNode
    )
{
    UNREFERENCED_PARAMETER(Urb);

    USB_BUILD_CONTROL_TRANSFER(
        &XID_Globals.EnumUrb.ControlTransfer,
        NULL,
        NULL,
        0,
        0,
        (PURB_COMPLETE_PROC)XID_EnumMouseComplete,
        (PVOID)XidNode,
        TRUE,
        USB_HOST_TO_DEVICE | USB_CLASS_COMMAND | USB_COMMAND_TO_INTERFACE,
        HID_SET_IDLE,
        HID_IDLE_INFINITE,
        XidNode->InterfaceNumber,
        0);

    XidNode->Device->SubmitRequest(&XID_Globals.EnumUrb);
}

VOID
XID_EnumMouseComplete(
    PURB Urb,
    PXID_DEVICE_NODE XidNode
    )
{
    UNREFERENCED_PARAMETER(Urb);

    XidNode->Type = XID_DEVTYPE_MOUSE;
    XidNode->SubType = 0;
    XidNode->bMaxInputReportSize = sizeof(XINPUT_MOUSE);
    XidNode->bMaxOutputReportSize = 0;
    XidNode->Device->SetClassSpecificType(XidNode->Type);
    XidNode->Device->AddComplete(USBD_STATUS_SUCCESS);
    XidNode->Ready = TRUE;
}

#endif /* DEBUG_MOUSE */
