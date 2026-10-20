#include "bridge_usb.h"
/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Debug USB mouse (xmouse_dbg), compiled in by default. The mouse is enumerated
 * as a HID boot mouse alongside the keyboard (XID_EnumMouse and
 * XID_EnumMouseComplete live in xid.cpp next to the keyboard path so they can
 * share the file-local enumeration watchdog helpers); this file holds the
 * per-poll report parser registered as XID_TYPE_INFORMATION::pfnProcessNewData.
 * A title reads the result through XInputGetState as XINPUT_STATE::DebugMouse.
 */

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

static char AddSignedByteWithSaturation(char a, char b)
{
    int sum = (int)a + (int)b;
    if (sum < -128)
        return (char)-128;
    if (sum > 127)
        return 127;
    return (char)sum;
}

//
// Synchronous control transfer on the mouse's interface, as XID_fOpenDevice does:
// submit at DPC level, wait at passive level, come back to DPC level. FALSE if the
// mouse went away meanwhile.
//
static BOOL XID_MouseControl(PXID_OPEN_DEVICE OpenDevice, PXID_DEVICE_NODE XidNode,
                             UCHAR RequestType, UCHAR Request, USHORT Value,
                             PVOID Buffer, ULONG Length, UCHAR Direction)
{
    KEVENT event;
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    USB_BUILD_CONTROL_TRANSFER(
        &OpenDevice->Urb.ControlTransfer,
        NULL,                                 // default endpoint
        Buffer,
        Length,
        Direction,
        (PURB_COMPLETE_PROC)XID_SyncComplete,
        &event,
        TRUE,                                 // short transfer OK
        RequestType,
        Request,
        Value,
        XidNode->InterfaceNumber,
        Length);
    XidNode->Device->SubmitRequest(&OpenDevice->Urb);

    KeLowerIrql(PASSIVE_LEVEL);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    KeRaiseIrqlToDpcLevel();

    return NULL != OpenDevice->XidNode && !XidNode->PendingRemove;
}

//
// XInputOpen step for the mouse (XID_TYPE_INFORMATION::pfnOpenDevice), as the XDK's
// XID_MouseOpen: read the current input report with room for a full-size report. A
// boot-protocol report is at most 4 bytes (buttons, X, Y, wheel); a longer one means
// the mouse ignored the SET_PROTOCOL sent during enumeration and is still in report
// protocol, so it would overrun every 4-byte interrupt read. Ask again for boot
// protocol. The report itself is discarded: XID_fOpenDevice starts from the defaults.
//
BOOL __attribute__((fastcall)) XID_MouseOpen(PXID_OPEN_DEVICE OpenDevice)
{
    PXID_DEVICE_NODE xidNode = OpenDevice->XidNode;
    if (NULL == xidNode)
        return FALSE;

    if (!XID_MouseControl(OpenDevice, xidNode,
                          USB_DEVICE_TO_HOST | USB_CLASS_COMMAND | USB_COMMAND_TO_INTERFACE,
                          XID_COMMAND_GET_REPORT, 0x0100 /* input report 0 */,
                          OpenDevice->ReportForUrb, XID_MAXIMUM_REPORT_SIZE,
                          USB_TRANSFER_DIRECTION_IN))
        return FALSE;

    if (OpenDevice->Urb.ControlTransfer.TransferBufferLength > sizeof(XINPUT_MOUSE))
    {
        if (!XID_MouseControl(OpenDevice, xidNode,
                              USB_HOST_TO_DEVICE | USB_CLASS_COMMAND | USB_COMMAND_TO_INTERFACE,
                              HID_SET_PROTOCOL, HID_BOOT_PROTOCOL,
                              NULL, 0, 0))
            return FALSE;
    }
    return TRUE;
}

//
// Per-poll input handler. The OHCI poll has just deposited the raw HID boot-mouse
// report in ReportForUrb:
//   byte0 = buttons, byte1 = dX, byte2 = dY, byte3 = wheel (all signed).
// As the XDK's mouse.obj does: while the title has not read the previous report
// (ReportUnread), add this motion to it so none is lost between reads; once it
// has, this report replaces it. Nothing clears the motion on a read, so reading
// again without a new packet returns the last motion with the same
// dwPacketNumber -- titles compare dwPacketNumber, as the XDK's DebugMouse
// sample does.
//
void __attribute__((fastcall)) XID_ProcessMouseData(PXID_OPEN_DEVICE OpenDevice)
{
    PXINPUT_MOUSE pMouse = (PXINPUT_MOUSE)OpenDevice->Report;

    if (OpenDevice->ReportUnread)
    {
        pMouse->cMickeysX = AddSignedByteWithSaturation(pMouse->cMickeysX, (char)OpenDevice->ReportForUrb[1]);
        pMouse->cMickeysY = AddSignedByteWithSaturation(pMouse->cMickeysY, (char)OpenDevice->ReportForUrb[2]);
        pMouse->cWheel    = AddSignedByteWithSaturation(pMouse->cWheel, (char)OpenDevice->ReportForUrb[3]);
        pMouse->bButtons  = OpenDevice->ReportForUrb[0];
    }
    else
    {
        // The bytes the mouse sent (a boot mouse without a wheel sends three);
        // XID_NewInterruptData has already capped the length at the report size.
        RtlCopyMemory(OpenDevice->Report, OpenDevice->ReportForUrb,
                      OpenDevice->Urb.BulkOrInterruptTransfer.TransferBufferLength);
    }

    XAutoPowerDownResetTimer();
}
