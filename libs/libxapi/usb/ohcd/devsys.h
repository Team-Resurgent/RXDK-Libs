/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Macros wrapping the development-system-only OHCD operations; they expand to
 * function calls when PERFORM_DEVSYS_OPERATIONS is defined and to nothing
 * otherwise.
 */

#pragma once
#define __DEVSYS_H__


#ifdef PERFORM_DEVSYS_OPERATIONS
//
//  If development system only operations are on
//  the macros call functions
//

#define OHCD_DEVSYS_CHECK_HARDWARE(_DeviceExtension_)   OHCD_DevSysCheckHardware(_DeviceExtension_);
#define OHCD_DEVSYS_TAKE_CONTROL(_DeviceExtension_) OHCD_DevSysTakeControl(_DeviceExtension_);

// 
// The functions behind the macros
//
VOID
OHCD_DevSysCheckHardware(
    IN POHCD_DEVICE_EXTENSION DeviceExtension
    );

VOID
OHCD_DevSysTakeControl(
    IN POHCD_DEVICE_EXTENSION DeviceExtension
    );

#else

//
//  If development system only operations are off
//  the macros are NOPs.
//
#define OHCD_DEVSYS_CHECK_HARDWARE(_DeviceExtension_)
#define OHCD_DEVSYS_TAKE_CONTROL(_DeviceExtension_)

#endif


