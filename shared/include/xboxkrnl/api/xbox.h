/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Xbox* kernel data exports: console-wide state a title can read directly. The
 * inter-title launch data page, the console's per-box cryptographic keys, the
 * hardware description, and the kernel version. These are imported variables,
 * not functions.
 */

#ifndef XBOXKRNL_API_XBOX_H
#define XBOXKRNL_API_XBOX_H

/* Pointer to the launch data page carrying parameters passed between titles
 * across a relaunch (NULL when the title was not launched with data). */
XBAPI PLAUNCH_DATA_PAGE LaunchDataPage;

/* Table of alternate per-console signature keys. */
XBAPI XBOX_KEY_DATA XboxAlternateSignatureKeys[];

/* Key used to decrypt the console EEPROM contents. */
XBAPI XBOX_KEY_DATA XboxEEPROMKey;

/* Console hardware description (flags, encoder, RAM configuration). */
XBAPI XBOX_HARDWARE_INFO XboxHardwareInfo;

/* Per-console hard-disk (ATA) unlock key. */
XBAPI XBOX_KEY_DATA XboxHDKey;

/* Running kernel version (major/minor/build/qfe). */
XBAPI XBOX_KRNL_VERSION XboxKrnlVersion;

/* Per-console network (LAN) key. */
XBAPI XBOX_KEY_DATA XboxLANKey;

/* Per-console content signature key. */
XBAPI XBOX_KEY_DATA XboxSignatureKey;

#endif
