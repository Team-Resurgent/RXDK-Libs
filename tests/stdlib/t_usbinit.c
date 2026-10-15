/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* USB device enumeration runs DPCs: the host controller's interrupt DPC and the
   USBD, hub and XID enumeration watchdog timers. Five xAPI DPC routines were
   compiled cdecl although the kernel (and the SDK's FPU-saving DPC wrapper)
   calls them stdcall; through the wrapper the stack came back 16 bytes off and
   the console bugchecked with IRQL_NOT_LESS_OR_EQUAL as soon as a gamepad was
   enumerated -- seen on iND-BiOS, where those DPCs fire, and not on the debug
   BIOS. This brings the peripheral stack up, lets enumeration run, opens the
   gamepads it found, and checks the title is still running. Original Xbox only;
   run it on a retail-kernel BIOS (e.g. iND-BiOS) too. */
#include "rxdk_test.h"
#include <xtl.h>

int main(void)
{
    DWORD inserted = 0, removed = 0, mask, opened = 0, failed = 0;
    int i, port;

    XInitDevices(0, NULL);                  /* default device types, as XBApp does */
    for (i = 0; i < 30; i++) {              /* ~3 s: enumeration and its watchdogs run */
        DWORD ins, rem;
        if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &ins, &rem)) {
            inserted |= ins;
            removed |= rem;
        }
        Sleep(100);
    }
    mask = XGetDevices(XDEVICE_TYPE_GAMEPAD);
    for (port = 0; port < 4; port++) {
        HANDLE h;
        if (!(mask & (1u << port)))
            continue;
        h = XInputOpen(XDEVICE_TYPE_GAMEPAD, port, XDEVICE_NO_SLOT, NULL);
        if (h) {
            XINPUT_STATE st;
            if (XInputGetState(h, &st) == ERROR_SUCCESS)
                opened++;
            else
                failed++;
            XInputClose(h);
        } else {
            failed++;
        }
    }
    for (i = 0; i < 10; i++)                /* let the XID report DPCs run with them open */
        Sleep(100);

    DbgPrint("[usbinit] gamepads present 0x%lx, opened %lu\n", mask, opened);
    CHECK(1, "title still running after USB enumeration and its DPCs");
    CHECK_EQI(failed, 0, "every enumerated gamepad opens and reports its state");

    CHECK_DONE("usbinit");
    return 0;
}
