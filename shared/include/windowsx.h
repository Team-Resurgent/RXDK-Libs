/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * <windowsx.h> - minimal subset of the Win32 helper macros actually used by the
 * RXDK libraries (the DirectMusic synth). The full Win32 windowsx.h is a large
 * grab-bag of window/GDI message crackers irrelevant to the Xbox; only the
 * global-memory pointer helpers and MAKEPOINTS are referenced here. Vendored so
 * RXDK is self-contained under the LLVM toolchain (which provides no Windows
 * windowsx.h); previously zig's bundled copy resolved it. See
 * docs/llvm-toolchain-plan.md.
 */
#ifndef __RXDK_WINDOWSX_H__
#define __RXDK_WINDOWSX_H__

/* GlobalAlloc/-Lock pointer helpers (GMEM_* flags come from the memory API) */
#define GlobalPtrHandle(lp) \
    ((HGLOBAL)GlobalHandle(lp))

#define GlobalLockPtr(lp) \
    ((BOOL)(ULONG_PTR)GlobalLock(GlobalPtrHandle(lp)))

#define GlobalUnlockPtr(lp) \
    GlobalUnlock(GlobalPtrHandle(lp))

#define GlobalAllocPtr(flags, cb) \
    (GlobalLock(GlobalAlloc((flags), (cb))))

#define GlobalReAllocPtr(lp, cbNew, flags) \
    (GlobalUnlockPtr(lp), GlobalLock(GlobalReAlloc(GlobalPtrHandle(lp), (cbNew), (flags))))

#define GlobalFreePtr(lp) \
    (GlobalUnlockPtr(lp), (BOOL)(ULONG_PTR)GlobalFree(GlobalPtrHandle(lp)))

/* extract a POINTS from an LPARAM */
#define MAKEPOINTS(l) \
    (*((POINTS *)&(l)))

#endif /* __RXDK_WINDOWSX_H__ */
