#include "bridge_usb.h"
/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
// Per-poll input handler. The OHCI poll has just deposited the raw HID boot-mouse
// report in ReportForUrb:
//   byte0 = buttons, byte1 = dX, byte2 = dY, byte3 = wheel (all signed).
// Accumulate the relative motion into the XINPUT_MOUSE the title reads via
// XInputGetState (which resets the deltas after each read).
//
void __attribute__((fastcall)) XID_ProcessMouseData(PXID_OPEN_DEVICE OpenDevice)
{
    PXINPUT_MOUSE pMouse = (PXINPUT_MOUSE)OpenDevice->Report;

    pMouse->cMickeysX = AddSignedByteWithSaturation(pMouse->cMickeysX, (char)OpenDevice->ReportForUrb[1]);
    pMouse->cMickeysY = AddSignedByteWithSaturation(pMouse->cMickeysY, (char)OpenDevice->ReportForUrb[2]);
    pMouse->cWheel    = AddSignedByteWithSaturation(pMouse->cWheel, (char)OpenDevice->ReportForUrb[3]);
    pMouse->bButtons  = OpenDevice->ReportForUrb[0];

    XAutoPowerDownResetTimer();
}
