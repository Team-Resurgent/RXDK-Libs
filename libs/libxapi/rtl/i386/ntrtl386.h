/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * i386-specific portion of ntrtlp.h: prototypes for the architecture-dependent
 * RTL helpers (context capture, exception-handler chain walking, return-address
 * retrieval). Included by ntrtlp.h on x86; these routines are statically linked
 * into the caller and usable from either kernel or user mode.
 */

//
// Exception handling procedure prototypes.
//
VOID
RtlpCaptureContext (
    OUT PCONTEXT ContextRecord
    );

VOID
RtlpUnlinkHandler (
    PEXCEPTION_REGISTRATION_RECORD UnlinkPointer
    );

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead (
    VOID
    );

PVOID
RtlpGetReturnAddress (
    VOID
    );


//
//  Record dump procedures.
//

VOID
RtlpContextDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );

VOID
RtlpExceptionReportDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );

VOID
RtlpExceptionRegistrationDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );
