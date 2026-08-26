/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Rtl* kernel imports: the runtime library. Self-contained helpers with no
 * device state -- counted ANSI/UNICODE_STRING manipulation and conversion,
 * memory fill/copy/compare, integer<->string formatting, user-mode critical
 * sections, 64-bit and byte-swap arithmetic, time-field conversion, exception
 * raising and stack walking. STRING/UNICODE_STRING outputs that "allocate" must
 * be released with the matching RtlFree* call.
 */

#ifndef XBOXKRNL_API_RTL_H
#define XBOXKRNL_API_RTL_H

/* Converts a counted ANSI string to UNICODE_STRING. When
 * AllocateDestinationString is TRUE the buffer is allocated (free with
 * RtlFreeUnicodeString). */
NTSTATUS STDCALL RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PSTRING SourceString,
    BOOLEAN AllocateDestinationString);

/* Appends Source to the counted Destination string, respecting its maximum
 * length. */
NTSTATUS STDCALL RtlAppendStringToString(
    IN PSTRING Destination,
    IN PSTRING Source);

/* Appends one UNICODE_STRING to another, respecting the maximum length. */
NTSTATUS STDCALL RtlAppendUnicodeStringToString(
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source);

/* Appends a null-terminated wide string to a UNICODE_STRING. */
NTSTATUS STDCALL RtlAppendUnicodeToString(
    PUNICODE_STRING Destination,
    PCWSTR Source);

/* Reports a failed assertion (expression, file, line, optional message) to the
 * debugger. */
VOID STDCALL RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL);

/* Captures the current register/execution context into ContextRecord. */
VOID STDCALL RtlCaptureContext(
    OUT PCONTEXT ContextRecord);

/* Captures up to FramesToCapture return addresses (skipping FramesToSkip) into
 * BackTrace; returns the number captured and an optional hash. */
USHORT STDCALL RtlCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID* BackTrace,
    OUT PULONG BackTraceHash);

/* Parses String (in the given Base, or auto-detected) to an integer in *Value. */
NTSTATUS STDCALL RtlCharToInteger(
    IN PCSZ String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value);

/* Returns the count of leading bytes that match between two buffers. */
SIZE_T STDCALL RtlCompareMemory(
    IN CONST VOID* Source1,
    IN CONST VOID* Source2,
    IN SIZE_T Length);

/* Returns the count of leading bytes of Source that match the repeated ULONG
 * Pattern. */
SIZE_T STDCALL RtlCompareMemoryUlong(
    PVOID Source,
    SIZE_T Length,
    ULONG Pattern);

/* Lexically compares two counted ANSI strings; returns <0/0/>0. */
LONG STDCALL RtlCompareString(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive);

/* Lexically compares two UNICODE_STRINGs; returns <0/0/>0. */
LONG STDCALL RtlCompareUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive);

/* Copies a counted ANSI string, truncating to the destination's maximum length. */
VOID STDCALL RtlCopyString(
    OUT PSTRING DestinationString,
    IN PSTRING SourceString);

/* Copies a UNICODE_STRING, truncating to the destination's maximum length. */
VOID STDCALL RtlCopyUnicodeString(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString);

/* Allocates and initializes a UNICODE_STRING from a wide C string. Free with
 * RtlFreeUnicodeString. Returns FALSE on allocation failure. */
BOOLEAN STDCALL RtlCreateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString);

/* Returns the lowercase form of a single wide character. */
WCHAR STDCALL RtlDowncaseUnicodeChar(
    WCHAR SourceCharacter);

/* Lowercases a UNICODE_STRING; may allocate the destination (free with
 * RtlFreeUnicodeString). */
NTSTATUS STDCALL RtlDowncaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString);

/* Acquires a user-mode critical section, blocking until owned. */
VOID STDCALL RtlEnterCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection);

/* Acquires a critical section and enters a critical region (defers APCs) in one
 * step. */
VOID STDCALL RtlEnterCriticalSectionAndRegion(
    PRTL_CRITICAL_SECTION CriticalSection);

/* Returns TRUE if two counted ANSI strings are equal. */
BOOLEAN STDCALL RtlEqualString(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive);

/* Returns TRUE if two UNICODE_STRINGs are equal. */
BOOLEAN STDCALL RtlEqualUnicodeString(
    IN CONST PUNICODE_STRING String1,
    IN CONST PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive);

/* 64-bit x 32-bit signed multiply returning a 64-bit product. */
LARGE_INTEGER STDCALL RtlExtendedIntegerMultiply(
    LARGE_INTEGER Multiplicand,
    LONG Multiplier);

/* 64-bit by 32-bit unsigned divide; the remainder is returned in *Remainder. */
LARGE_INTEGER STDCALL RtlExtendedLargeIntegerDivide(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder);

/* Reciprocal-multiply divide used to divide by a compile-time constant via a
 * precomputed magic number and shift. */
LARGE_INTEGER STDCALL RtlExtendedMagicDivide(
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount);

/* Fills Length bytes at Destination with the byte Fill. */
VOID STDCALL RtlFillMemory(
    PVOID Destination,
    ULONG Length,
    UCHAR Fill);

/* Fills a region with the repeated ULONG Pattern (Length in bytes). */
VOID STDCALL RtlFillMemoryUlong(
    PVOID Destination,
    SIZE_T Length,
    ULONG Pattern);

/* Frees an ANSI string buffer allocated by an Rtl conversion routine. */
VOID STDCALL RtlFreeAnsiString(
    PANSI_STRING AnsiString);

/* Frees a UNICODE_STRING buffer allocated by an Rtl conversion routine. */
VOID STDCALL RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString);

/* Returns the caller's and caller's-caller return addresses. */
VOID STDCALL RtlGetCallersAddress(
    OUT PVOID* CallersAddress,
    OUT PVOID* CallersCaller);

/* Initializes an ANSI_STRING to reference a null-terminated C string (no copy). */
VOID STDCALL RtlInitAnsiString(
    PANSI_STRING DestinationString,
    IN PCSZ SourceString);

/* Initializes a critical section to the unowned state. */
VOID STDCALL RtlInitializeCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection);

/* The Xbox kernel exports no RtlDeleteCriticalSection (there is no such ordinal) — a
   critical section owns no allocated resources, so teardown is a no-op. Provided inline
   so code using it (and winbase.h's DeleteCriticalSection macro) compiles and links. */
static __inline VOID
RtlDeleteCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection)
{
    (void)CriticalSection;
}

/* Initializes a UNICODE_STRING to reference a null-terminated wide string (no
 * copy). */
VOID STDCALL RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString);

/* Formats Value in the given Base into the C-string buffer String (OutputLength
 * bytes). */
NTSTATUS STDCALL RtlIntegerToChar(
    ULONG Value,
    ULONG Base,
    LONG OutputLength,
    PSZ String);

/* Formats Value in the given Base into a UNICODE_STRING. */
NTSTATUS STDCALL RtlIntegerToUnicodeString(
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String);

/* Releases a critical section acquired with RtlEnterCriticalSection. */
VOID STDCALL RtlLeaveCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection);

/* Releases a critical section and leaves the critical region in one step. */
VOID STDCALL RtlLeaveCriticalSectionAndRegion(
    PRTL_CRITICAL_SECTION CriticalSection);

/* Returns the lowercase form of a single ANSI character. */
CHAR STDCALL RtlLowerChar(
    IN CHAR Character);

/* Maps the generic bits of an access mask to specific rights using
 * GenericMapping. */
VOID STDCALL RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping);

/* Copies Length bytes, handling overlap between Source and Destination. */
VOID STDCALL RtlMoveMemory(
    PVOID Destination,
    CONST PVOID Source,
    ULONG Length);

/* Converts a multibyte string to wide characters using the OEM code page; bytes
 * written are returned in *BytesInUnicodeString. */
NTSTATUS STDCALL RtlMultiByteToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString);

/* Computes the wide-character byte size a multibyte string would convert to. */
NTSTATUS STDCALL RtlMultiByteToUnicodeSize(
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString);

/* Maps an NTSTATUS to the equivalent Win32/DOS error code. */
ULONG STDCALL RtlNtStatusToDosError(
    IN NTSTATUS Status);

/* Raises a structured exception described by ExceptionRecord. */
VOID STDCALL RtlRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord);

/* Raises an exception carrying just the NTSTATUS Status. */
VOID STDCALL RtlRaiseStatus(
    IN NTSTATUS Status);

/* Reports a "raise-in-place" API misuse (name/expression/message) to the
 * debugger. */
VOID STDCALL RtlRip(
    IN PVOID ApiName,
    IN PVOID Expression,
    IN PVOID Message);

/* Bounded formatted print into a buffer of the given size (printf semantics). */
VOID CDECL RtlSnprintf(
    CHAR*,
    SIZE_T,
    CONST CHAR*,
    ...);

/* Unbounded formatted print into a buffer (printf semantics). */
VOID CDECL RtlSprintf(
    CHAR*,
    CONST CHAR*,
    ...);

/* Converts broken-down TIME_FIELDS to a 64-bit system time; returns FALSE if
 * the fields are invalid. */
BOOLEAN STDCALL RtlTimeFieldsToTime(
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time);

/* Converts a 64-bit system time to broken-down TIME_FIELDS. */
VOID STDCALL RtlTimeToTimeFields(
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields);

/* Attempts to acquire a critical section without blocking; returns TRUE if
 * acquired. */
BOOLEAN STDCALL RtlTryEnterCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection);

/* Byte-reverses a 32-bit value (endian swap). Fast call. */
ULONG FASTCALL RtlUlongByteSwap(
    IN ULONG Source);

/* Converts a UNICODE_STRING to a counted ANSI string; may allocate the
 * destination (free with RtlFreeAnsiString). */
NTSTATUS STDCALL RtlUnicodeStringToAnsiString(
    OUT PSTRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString);

/* Parses a UNICODE_STRING in the given Base to an integer in *Value. */
NTSTATUS STDCALL RtlUnicodeStringToInteger(
    PUNICODE_STRING String,
    ULONG Base,
    PULONG Value);

/* Converts wide characters to a multibyte string using the OEM code page. */
NTSTATUS STDCALL RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString);

/* Computes the multibyte byte size a wide string would convert to. */
NTSTATUS STDCALL RtlUnicodeToMultiByteSize(
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString);

/* Performs an exception unwind to TargetFrame/TargetIp, running termination
 * handlers along the way. */
VOID STDCALL RtlUnwind(
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue);

/* Returns the uppercase form of a single wide character. */
WCHAR STDCALL RtlUpcaseUnicodeChar(
    WCHAR SourceCharacter);

/* Uppercases a UNICODE_STRING; may allocate the destination (free with
 * RtlFreeUnicodeString). */
NTSTATUS STDCALL RtlUpcaseUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString);

/* Uppercases wide characters and converts them to a multibyte string in one
 * step. */
NTSTATUS STDCALL RtlUpcaseUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString);

/* Returns the uppercase form of a single ANSI character. */
CHAR STDCALL RtlUpperChar(
    CHAR Character);

/* Uppercases a counted ANSI string into the destination. */
VOID STDCALL RtlUpperString(
    PSTRING DestinationString,
    PSTRING SourceString);

/* Byte-reverses a 16-bit value (endian swap). Fast call. */
USHORT FASTCALL RtlUshortByteSwap(
    IN USHORT Source);

/* Bounded formatted print with a va_list argument (vsnprintf semantics). */
VOID CDECL RtlVsnprintf(
    CHAR*,
    SIZE_T,
    CONST CHAR*,
    ...);

/* Unbounded formatted print with a va_list argument (vsprintf semantics). */
VOID CDECL RtlVsprintf(
    CHAR*,
    CONST CHAR*,
    ...);

/* Walks the current call stack, storing up to Count return addresses in
 * Callers; returns the number captured. */
ULONG STDCALL RtlWalkFrameChain(
    OUT PVOID* Callers,
    IN ULONG Count,
    IN ULONG Flags);

/* Zeroes Length bytes at Destination. */
VOID STDCALL RtlZeroMemory(
    IN VOID UNALIGNED* Destination,
    IN SIZE_T Length);

#endif
