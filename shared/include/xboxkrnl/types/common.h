/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Common kernel ABI macros and small shared types. Defines the SAL-style
 * parameter annotations, the calling-convention and import attributes (STDCALL,
 * FASTCALL, XBAPI, ...), the NTSTATUS test macros, IRQL levels, the AV video
 * pack identifiers, UNICODE_STRING, and the intrusive linked-list helper macros.
 * Included first by <xboxkrnl/types.h>.
 */

#ifndef XBOXKRNL_TYPES_COMMON_H
#define XBOXKRNL_TYPES_COMMON_H

/* SAL-style parameter annotations. These expand to nothing; they document
 * direction (IN/OUT), optionality, and alignment for the reader and tooling. */
#define IN
#define OUT
#define UNALIGNED
#define OPTIONAL
/* extern for the MSVC IntelliSense parser so `XBAPI CONST <t> Var;` reads as a declaration,
   not a definition (C++ gives const file-scope internal linkage). The clang build is unchanged. */
#if defined(_MSC_VER) && !defined(__clang__)
#define XBAPI extern __declspec(dllimport)
#else
#define XBAPI __declspec(dllimport)
#endif
/* Calling-convention and declaration attributes applied to the kernel imports:
 * STDCALL/NTAPI (callee-cleaned), CDECL (caller-cleaned, varargs), FASTCALL
 * (register args), the no-return marker, restrict, and const. */
#define STDCALL __attribute__((__stdcall__))
#define NTAPI STDCALL
// Keyword form, matching windef.h's fallback so the two definitions are identical
// (this one is unguarded, so include order would otherwise decide which wins).
#define CDECL __cdecl
#define FASTCALL __attribute__((fastcall))
#define DECLSPEC_NORETURN __attribute__((noreturn))
#define RESTRICTED_POINTER __restrict__
#define CONST const
#ifndef NULL
#define NULL ((PVOID)0)
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* NTSTATUS severity tests: NT_SUCCESS is true for success and informational
 * codes; the others isolate the two high severity bits. */
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)
/* AV pack (video connector) identifiers reported by the AV/HAL layer. */
#define AV_PACK_NONE 0x00000000
#define AV_PACK_STANDARD 0x00000001
#define AV_PACK_RFU 0x00000002
#define AV_PACK_SCART 0x00000003
#define AV_PACK_HDTV 0x00000004
#define AV_PACK_VGA 0x00000005
#define AV_PACK_SVIDEO 0x00000006

typedef ULONG PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef UCHAR KIRQL, *PKIRQL;

/* Interrupt request levels (KIRQL). Code runs at PASSIVE_LEVEL normally; raising
 * to APC_LEVEL blocks APCs and to DISPATCH_LEVEL blocks the scheduler. Many
 * kernel routines require a maximum IRQL. */
#define PASSIVE_LEVEL 0
#define APC_LEVEL 1
#define DISPATCH_LEVEL 2

typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG KPRIORITY;
typedef ULONG DEVICE_TYPE;
typedef ULONG LOGICAL;
typedef ULONG ACCESS_MASK, *PACCESS_MASK;
typedef CCHAR KPROCESSOR_MODE;

typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

/* Builds a STRING/UNICODE_STRING initializer from a string literal at compile
 * time (Length excludes, MaximumLength includes, the terminator). */
#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), (s) }

typedef STRING OBJECT_STRING, *POBJECT_STRING;

/* Counted wide-character string: the UTF-16 counterpart of STRING, with byte
 * counts in Length/MaximumLength. */
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef CONST UNICODE_STRING* PCUNICODE_STRING;

/* Intrusive linked-list helpers operating on LIST_ENTRY (doubly linked) and
 * SINGLE_LIST_ENTRY (singly linked) nodes: init/empty tests and O(1)
 * insert/remove at either end. */
#define InitializeListHead(ListHead) ((ListHead)->Flink = (ListHead)->Blink = (ListHead))
#define IsListEmpty(ListHead) ((ListHead)->Flink == (ListHead))
#define RemoveHeadList(ListHead) \
    (ListHead)->Flink; \
    { RemoveEntryList((ListHead)->Flink) }
#define RemoveTailList(ListHead) \
    (ListHead)->Blink; \
    { RemoveEntryList((ListHead)->Blink) }
#define RemoveEntryList(Entry) \
    { \
        PLIST_ENTRY _EX_Blink; \
        PLIST_ENTRY _EX_Flink; \
        _EX_Flink = (Entry)->Flink; \
        _EX_Blink = (Entry)->Blink; \
        _EX_Blink->Flink = _EX_Flink; \
        _EX_Flink->Blink = _EX_Blink; \
    }

#define InsertTailList(ListHead, Entry) \
    { \
        PLIST_ENTRY _EX_Blink; \
        PLIST_ENTRY _EX_ListHead; \
        _EX_ListHead = (ListHead); \
        _EX_Blink = _EX_ListHead->Blink; \
        (Entry)->Flink = _EX_ListHead; \
        (Entry)->Blink = _EX_Blink; \
        _EX_Blink->Flink = (Entry); \
        _EX_ListHead->Blink = (Entry); \
    }

#define InsertHeadList(ListHead, Entry) \
    { \
        PLIST_ENTRY _EX_Flink; \
        PLIST_ENTRY _EX_ListHead; \
        _EX_ListHead = (ListHead); \
        _EX_Flink = _EX_ListHead->Flink; \
        (Entry)->Flink = _EX_Flink; \
        (Entry)->Blink = _EX_ListHead; \
        _EX_Flink->Blink = (Entry); \
        _EX_ListHead->Flink = (Entry); \
    }

#define PopEntryList(ListHead) \
    (ListHead)->Next; \
    { \
        PSINGLE_LIST_ENTRY FirstEntry; \
        FirstEntry = (ListHead)->Next; \
        if (FirstEntry != NULL) { \
            (ListHead)->Next = FirstEntry->Next; \
        } \
    }

#define PushEntryList(ListHead, Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry);

/* Recovers the containing struct pointer from a pointer to one of its fields. */
#define CONTAINING_RECORD(Address, Type, Field) ((Type*)((PCHAR)(Address) - (ULONG_PTR) __builtin_offsetof(Type, Field)))
/* Page size (bytes) and a helper to round a size up to a whole number of pages. */
#define PAGE_SIZE 4096
#define ROUND_TO_PAGES(Size) (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#endif
