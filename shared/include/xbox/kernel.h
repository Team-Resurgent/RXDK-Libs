/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Convenience umbrella over the Xbox kernel import surface shared by libc,
 * libcpp, and libxapi. Pulls in the full <xboxkrnl/xboxkrnl.h> declaration set;
 * the linker resolves each referenced import against xboxkrnl.lib, so only the
 * symbols a title actually uses are drawn in.
 */

#ifndef RXDK_XBOX_KERNEL_H
#define RXDK_XBOX_KERNEL_H

#include <xboxkrnl/xboxkrnl.h>

#endif /* RXDK_XBOX_KERNEL_H */
