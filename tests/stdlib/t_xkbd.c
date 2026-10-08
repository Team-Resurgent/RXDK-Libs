/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* The USB keyboard queue (xkbd.h). libxapi is built single-keyboard and the
   header declares that form (XInputDebugGetKeystroke takes no device handle,
   exported as _XInputDebugGetKeystroke@4); linking checks they still agree. */
#include "rxdk_test.h"
#include <xtl.h>

int main(void)
{
    XINPUT_DEBUG_KEYSTROKE key;

    CHECK_EQI(XInputDebugInitKeyboardQueue(NULL), ERROR_SUCCESS, "XInputDebugInitKeyboardQueue(NULL)");
    /* No keyboard is attached in the test environment: the queue is empty. */
    CHECK_EQI(XInputDebugGetKeystroke(&key), ERROR_HANDLE_EOF, "XInputDebugGetKeystroke on an empty queue");

    CHECK_DONE("xkbd");
    return 0;
}
