/*
 * RXDK: single PE TLS image template for Xbox xAPI (Clang/lld).
 *
 * All per-thread slots live in one translation unit with explicit .tls$ section
 * names so lld merges them between _tls_start and _tls_end. Scatter-gather
 * __declspec(thread) TUs with -fdata-sections produce orphan comdat .tls$$
 * sections that never reach IMAGE_TLS_DIRECTORY (8-byte template bug).
 */

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
