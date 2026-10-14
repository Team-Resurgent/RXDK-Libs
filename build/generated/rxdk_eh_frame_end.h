/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Force-included into libunwind (build/sdk/libcpp.json). libunwind finds a
 * title's unwind tables only between __eh_frame_start and __eh_frame_end; it no
 * longer scans memory to make up for an end marker that leaves FDEs outside.
 * RXDK Tools 1.3.6 and later put the end marker in .eh_frame$z, after every
 * contribution, check the bracket after linking, and define it under this name
 * as well. Older Tools put it where archive members' FDEs land after it.
 *
 * Referring to the end marker by this name makes a link with older Tools fail
 * with an undefined symbol naming the requirement, instead of building a title
 * whose throws terminate at run time.
 */
#ifndef RXDK_EH_FRAME_END_H
#define RXDK_EH_FRAME_END_H

#define __eh_frame_end __rxdk_eh_frame_end_requires_rxdk_tools_1_3_6

#endif
