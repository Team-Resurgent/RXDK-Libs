/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Base platform header for RXDK titles. Establishes the compile-time platform
 * identity macros (_XBOX / _WIN32 / _X86_) that the rest of the SDK keys off of,
 * and declares the two entry-point helpers the C runtime startup relies on.
 */

#ifndef RXDK_XBOX_H
#define RXDK_XBOX_H

/* Platform identity: RXDK targets the original Xbox, a 32-bit x86 Win32-style ABI. */
#define _XBOX 1
#define _WIN32 1
#define _X86_ 1

/* One-time C runtime / platform bring-up, called by the startup code before main. */
void xbox_runtime_init(void);

/* Stop the machine permanently (unrecoverable error path); never returns. */
void xbox_halt(void) __attribute__((noreturn));

#endif /* RXDK_XBOX_H */
