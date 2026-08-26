/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef RXDK_XAPI_NT_BRIDGE_H
#define RXDK_XAPI_NT_BRIDGE_H

/*
 * Minimal nt.h / ntdef.h surface the ntos USB slices need, sourced from the
 * xboxkrnl headers.
 */

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/ntstatus.h>
#include <xboxkrnl/types/common.h>

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif

#ifndef NTKERNELAPI
#define NTKERNELAPI
#endif
#ifndef NTHALAPI
#define NTHALAPI
#endif

#ifndef FORCEINLINE
#define FORCEINLINE static __inline
#endif

#ifndef PULONG_PTR
typedef ULONG_PTR *PULONG_PTR;
#endif
#ifndef PLONG_PTR
typedef LONG_PTR *PLONG_PTR;
#endif

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PVOID Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

typedef struct _FX_SAVE_AREA {
    USHORT ControlWord;
    USHORT StatusWord;
    USHORT TagWord;
    USHORT ErrorOpcode;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    UCHAR RegisterArea[80];
} FX_SAVE_AREA, *PFX_SAVE_AREA;

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

#endif
