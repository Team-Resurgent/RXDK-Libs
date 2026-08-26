/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
