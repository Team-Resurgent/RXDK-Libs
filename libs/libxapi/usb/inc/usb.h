/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef RXDK_USB_H
#define RXDK_USB_H

/*
 * Xbox USB stack header — vendor sources include <usb.h>; usbd.h is the
 * private Xbox definition (not zig libc's Win32 usb.h).
 */

#ifdef __cplusplus
#include <usbd.h>
#else
#include <usb100.h>
#include <hcdi.h>
#endif

#endif /* RXDK_USB_H */
