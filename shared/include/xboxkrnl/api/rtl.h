#ifndef XBOXKRNL_API_RTL_H
#define XBOXKRNL_API_RTL_H

NTSTATUS STDCALL RtlAnsiStringToUnicodeString
(
    PUNICODE_STRING DestinationString,
    PSTRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSTATUS STDCALL RtlAppendStringToString
(
    IN PSTRING Destination,
    IN PSTRING Source
);

NTSTATUS STDCALL RtlAppendUnicodeStringToString
(
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source
);

NTSTATUS STDCALL RtlAppendUnicodeToString
(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

VOID STDCALL RtlAssert
(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
);

VOID STDCALL RtlCaptureContext
(
    OUT PCONTEXT ContextRecord
);

USHORT STDCALL RtlCaptureStackBackTrace
(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
);

NTSTATUS STDCALL RtlCharToInteger
(
    IN PCSZ String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value
);

SIZE_T STDCALL RtlCompareMemory
(
    IN CONST VOID *Source1,
    IN CONST VOID *Source2,
    IN SIZE_T Length
);

SIZE_T STDCALL RtlCompareMemoryUlong
(
    PVOID Source,
    SIZE_T Length,
    ULONG Pattern
);

LONG STDCALL RtlCompareString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

LONG STDCALL RtlCompareUnicodeString
(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
);

VOID STDCALL RtlCopyString
(
    OUT PSTRING DestinationString,
    IN PSTRING SourceString
);

VOID STDCALL RtlCopyUnicodeString
(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString
);

BOOLEAN STDCALL RtlCreateUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
);

WCHAR STDCALL RtlDowncaseUnicodeChar
(
    WCHAR SourceCharacter
);

NTSTATUS STDCALL RtlDowncaseUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

VOID STDCALL RtlEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

VOID STDCALL RtlEnterCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

BOOLEAN STDCALL RtlEqualString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

BOOLEAN STDCALL RtlEqualUnicodeString
(
    IN CONST PUNICODE_STRING String1,
    IN CONST PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
);

LARGE_INTEGER STDCALL RtlExtendedIntegerMultiply
(
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
);

LARGE_INTEGER STDCALL RtlExtendedLargeIntegerDivide
(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
);

LARGE_INTEGER STDCALL RtlExtendedMagicDivide
(
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
);

VOID STDCALL RtlFillMemory
(
    PVOID Destination,
    ULONG Length,
    UCHAR Fill
);

VOID STDCALL RtlFillMemoryUlong
(
    PVOID Destination,
    SIZE_T Length,
    ULONG Pattern
);

VOID STDCALL RtlFreeAnsiString
(
    PANSI_STRING AnsiString
);

VOID STDCALL RtlFreeUnicodeString
(
    PUNICODE_STRING UnicodeString
);

VOID STDCALL RtlGetCallersAddress
(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
);

VOID STDCALL RtlInitAnsiString
(
    PANSI_STRING DestinationString,
    IN PCSZ SourceString
);

VOID STDCALL RtlInitializeCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

VOID STDCALL RtlInitUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

NTSTATUS STDCALL RtlIntegerToChar
(
    ULONG Value,
    ULONG Base,
    LONG OutputLength,
    PSZ String
);

NTSTATUS STDCALL RtlIntegerToUnicodeString
(
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String
);

VOID STDCALL RtlLeaveCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

VOID STDCALL RtlLeaveCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

CHAR STDCALL RtlLowerChar
(
    IN CHAR Character
);

VOID STDCALL RtlMapGenericMask
(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

VOID STDCALL RtlMoveMemory
(
    PVOID Destination,
    CONST PVOID Source,
    ULONG Length
);

NTSTATUS STDCALL RtlMultiByteToUnicodeN
(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

NTSTATUS STDCALL RtlMultiByteToUnicodeSize
(
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

ULONG STDCALL RtlNtStatusToDosError
(
    IN NTSTATUS Status
);

VOID STDCALL RtlRaiseException
(
    IN PEXCEPTION_RECORD ExceptionRecord
);

VOID STDCALL RtlRaiseStatus
(
    IN NTSTATUS Status
);

VOID STDCALL RtlRip
(
    IN PVOID ApiName,
    IN PVOID Expression,
    IN PVOID Message
);

VOID CDECL RtlSnprintf
(
    CHAR *,
    SIZE_T,
    CONST CHAR *,
    ...
);

VOID CDECL RtlSprintf
(
    CHAR *,
    CONST CHAR *,
    ...
);

BOOLEAN STDCALL RtlTimeFieldsToTime
(
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
);

VOID STDCALL RtlTimeToTimeFields
(
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
);

BOOLEAN STDCALL RtlTryEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

ULONG FASTCALL RtlUlongByteSwap
(
    IN ULONG Source
);

NTSTATUS STDCALL RtlUnicodeStringToAnsiString
(
    OUT PSTRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

NTSTATUS STDCALL RtlUnicodeStringToInteger
(
    PUNICODE_STRING String,
    ULONG Base,
    PULONG Value
);

NTSTATUS STDCALL RtlUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

NTSTATUS STDCALL RtlUnicodeToMultiByteSize
(
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

VOID STDCALL RtlUnwind
(
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
);

WCHAR STDCALL RtlUpcaseUnicodeChar
(
    WCHAR SourceCharacter
);

NTSTATUS STDCALL RtlUpcaseUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSTATUS STDCALL RtlUpcaseUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

CHAR STDCALL RtlUpperChar
(
    CHAR Character
);

VOID STDCALL RtlUpperString
(
    PSTRING DestinationString,
    PSTRING SourceString
);

USHORT FASTCALL RtlUshortByteSwap
(
    IN USHORT Source
);

VOID CDECL RtlVsnprintf
(
    CHAR *,
    SIZE_T,
    CONST CHAR*,
    ...
);

VOID CDECL RtlVsprintf
(
    CHAR *,
    CONST CHAR*,
    ...
);

ULONG STDCALL RtlWalkFrameChain
(
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
);

VOID STDCALL RtlZeroMemory
(
    IN VOID UNALIGNED *Destination,
    IN SIZE_T Length
);

#endif
