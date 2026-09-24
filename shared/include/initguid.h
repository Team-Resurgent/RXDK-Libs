/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * <initguid.h> - the MS pattern for emitting GUID storage: define INITGUID and
 * re-include guiddef.h so DEFINE_GUID flips from an extern declaration to an
 * allocating definition (libxapi/nt/guiddef.h keys off INITGUID). A TU includes
 * this before the headers whose GUIDs it wants to define in this object.
 *
 * Vendored so RXDK is self-contained under the LLVM toolchain (which supplies no
 * Windows <initguid.h>); previously the zig toolchain's bundled copy resolved
 * it. See docs/llvm-toolchain-plan.md. (libdmusic has its own equivalent copy on
 * its private include path.)
 */
#ifndef INITGUID
#define INITGUID
#endif
#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif
#include <guiddef.h>
