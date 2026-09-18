/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * <share.h> - the standard C runtime file-sharing mode flags (_SH_*) passed to
 * _fsopen / _sopen. The DirectMusic loader/synth reference _SH_DENYNO and
 * _SH_DENYWR. Vendored so RXDK is self-contained under the LLVM toolchain (which
 * provides no Windows share.h); previously zig's bundled copy resolved it. See
 * docs/llvm-toolchain-plan.md.
 */
#ifndef __RXDK_SHARE_H__
#define __RXDK_SHARE_H__

#define _SH_DENYRW 0x10 /* deny read/write mode */
#define _SH_DENYWR 0x20 /* deny write mode */
#define _SH_DENYRD 0x30 /* deny read mode */
#define _SH_DENYNO 0x40 /* deny none mode */
#define _SH_SECURE 0x80 /* secure mode */

#endif /* __RXDK_SHARE_H__ */
