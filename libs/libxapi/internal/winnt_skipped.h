/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

#pragma once
#define RXDK_XAPI_WINNT_SKIPPED_H


/*
 * Typedefs windef.h normally pulls from winnt.h when NT_INCLUDED is set,
 * provided here for the USB/kernel translation units that use sdk/nt.h instead
 * of xboxkrnl and so never see xboxdef.h.
 */

#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif


#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer) \
    ((ULONG_PTR)(ArgumentPointer) != (ULONG_PTR)(NULL))
#endif

#ifndef Int32x32To64
#define Int32x32To64(a, b) ((LONGLONG)((LONGLONG)(LONG)(a) * (LONG)(b)))
#endif
#ifndef UInt32x32To64
#define UInt32x32To64(a, b) ((ULONGLONG)((ULONGLONG)(DWORD)(a) * (DWORD)(b)))
#endif

#ifndef RtlInitObjectString
#define RtlInitObjectString RtlInitAnsiString
#endif

