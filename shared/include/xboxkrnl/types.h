/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Aggregator for the Xbox kernel type headers. Pulls in the common ABI macros
 * and primitive types, followed by the file/kernel/io/misc structure sets, in
 * an order that satisfies their inter-dependencies.
 */

#ifndef XBOXKRNL_TYPES_H
#define XBOXKRNL_TYPES_H

#include <xboxkrnl/types/common.h>
#include <xboxkrnl/types/file.h>
#include <xboxkrnl/types/kernel.h>
#include <xboxkrnl/types/io.h>
#include <xboxkrnl/types/misc.h>

#endif
