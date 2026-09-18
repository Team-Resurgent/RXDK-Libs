/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * <regstr.h> - minimal registry-path string constants referenced by the
 * DirectMusic port. The Xbox has no registry, so these paths are only used as
 * string literals in vestigial loader/synth code. RXDK's own dmusics.h already
 * defines REGSTR_PATH_SOFTWARESYNTHS; the guards below supply the standard
 * DirectMusic paths only if a prior header has not. Vendored so RXDK is
 * self-contained under the LLVM toolchain (which provides no Windows regstr.h);
 * previously zig's bundled copy resolved the include. See docs/llvm-toolchain-plan.md.
 */
#ifndef __RXDK_REGSTR_H__
#define __RXDK_REGSTR_H__

#ifndef REGSTR_PATH_SOFTWARESYNTHS
#define REGSTR_PATH_SOFTWARESYNTHS "Software\\Microsoft\\DirectMusic\\SoftwareSynths"
#endif

#ifndef REGSTR_PATH_TOOLS
#define REGSTR_PATH_TOOLS "Software\\Microsoft\\DirectMusic\\Tools"
#endif

#endif /* __RXDK_REGSTR_H__ */
