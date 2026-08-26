/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * The single PE TLS image template for the Xbox xAPI under Clang/lld, plus the
 * IMAGE_TLS_DIRECTORY (_tls_used) that describes it. Every per-thread slot is
 * defined in this one translation unit with explicit, ordered .tls section
 * names so lld lays them out contiguously between _tls_start and _tls_end;
 * spreading __declspec(thread) across TUs with -fdata-sections instead yields
 * orphan comdat TLS sections that never reach the TLS directory.
 */

#include "bridge_k32.h"

#include "dllp.h"
#pragma hdrstop

#include "xfiber.h"

extern ULONG _tls_index;

#pragma data_seg(".tls")
char _tls_start = 0;

#pragma data_seg(".tls$RXDK01_CURFIB")
__declspec(thread) PVOID RxdkXapiCurrentFiberTemplate;

#pragma data_seg(".tls$RXDK02_THRFIB")
__declspec(thread) XFIBER RxdkXapiThreadFiberDataTemplate;

#pragma data_seg(".tls$RXDK03_LASTERR")
__declspec(thread) DWORD XapiLastErrorCode = 0;

#pragma data_seg(".tls$RXDK04_SLOTS")
__declspec(thread) PVOID XapiTlsSlots[TLS_MINIMUM_AVAILABLE];

#pragma data_seg(".tls$ZZZ")
char _tls_end = 0;

#pragma comment(linker, "/SECTION:.tls,RW")

#pragma data_seg(".rdata$T")

const IMAGE_TLS_DIRECTORY _tls_used = {
    (ULONG)(ULONG_PTR)&_tls_start,
    (ULONG)(ULONG_PTR)&_tls_end,
    (ULONG)(ULONG_PTR)&_tls_index,
    (ULONG)(ULONG_PTR)NULL,
    (ULONG)0,
    (ULONG)0,
};
