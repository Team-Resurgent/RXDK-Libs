#ifndef XBOXKRNL_API_RTL_H
#define XBOXKRNL_API_RTL_H

XBAPI NTSTATUS NTAPI RtlAnsiStringToUnicodeString
(
    PUNICODE_STRING DestinationString,
    PSTRING SourceString,
    BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS NTAPI RtlAppendStringToString
(
    IN PSTRING Destination,
    IN PSTRING Source
);

XBAPI NTSTATUS NTAPI RtlAppendUnicodeStringToString
(
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source
);

XBAPI NTSTATUS NTAPI RtlAppendUnicodeToString
(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

XBAPI VOID NTAPI RtlAssert
(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
);

XBAPI VOID NTAPI RtlCaptureContext
(
    OUT PCONTEXT ContextRecord
);

XBAPI USHORT NTAPI RtlCaptureStackBackTrace
(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
);

XBAPI NTSTATUS NTAPI RtlCharToInteger
(
    IN PCSZ String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value
);

XBAPI SIZE_T NTAPI RtlCompareMemory
(
    IN CONST VOID *Source1,
    IN CONST VOID *Source2,
    IN SIZE_T Length
);

XBAPI SIZE_T NTAPI RtlCompareMemoryUlong
(
    PVOID Source,
    SIZE_T Length,
    ULONG Pattern
);

XBAPI LONG NTAPI RtlCompareString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI LONG NTAPI RtlCompareUnicodeString
(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
);

XBAPI VOID NTAPI RtlCopyString
(
    OUT PSTRING DestinationString,
    IN PSTRING SourceString
);

XBAPI VOID NTAPI RtlCopyUnicodeString
(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString
);

XBAPI BOOLEAN NTAPI RtlCreateUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
);

XBAPI WCHAR NTAPI RtlDowncaseUnicodeChar
(
    WCHAR SourceCharacter
);

XBAPI NTSTATUS NTAPI RtlDowncaseUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

XBAPI VOID NTAPI RtlEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID NTAPI RtlEnterCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI BOOLEAN NTAPI RtlEqualString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI BOOLEAN NTAPI RtlEqualUnicodeString
(
    IN CONST PUNICODE_STRING String1,
    IN CONST PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI LARGE_INTEGER NTAPI RtlExtendedIntegerMultiply
(
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
);

XBAPI LARGE_INTEGER NTAPI RtlExtendedLargeIntegerDivide
(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
);

XBAPI LARGE_INTEGER NTAPI RtlExtendedMagicDivide
(
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
);

XBAPI VOID NTAPI RtlFillMemory
(
    PVOID Destination,
    ULONG Length,
    UCHAR Fill
);

XBAPI VOID NTAPI RtlFillMemoryUlong
(
    PVOID Destination,
    SIZE_T Length,
    ULONG Pattern
);

XBAPI VOID NTAPI RtlFreeAnsiString
(
    PANSI_STRING AnsiString
);

XBAPI VOID NTAPI RtlFreeUnicodeString
(
    PUNICODE_STRING UnicodeString
);

XBAPI VOID NTAPI RtlGetCallersAddress
(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
);

XBAPI VOID NTAPI RtlInitAnsiString
(
    PANSI_STRING DestinationString,
    IN PCSZ SourceString
);

XBAPI VOID NTAPI RtlInitializeCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID NTAPI RtlInitUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

XBAPI NTSTATUS NTAPI RtlIntegerToChar
(
    ULONG Value,
    ULONG Base,
    LONG OutputLength,
    PSZ String
);

XBAPI NTSTATUS NTAPI RtlIntegerToUnicodeString
(
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String
);

XBAPI VOID NTAPI RtlLeaveCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID NTAPI RtlLeaveCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI CHAR NTAPI RtlLowerChar
(
    IN CHAR Character
);

XBAPI VOID NTAPI RtlMapGenericMask
(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

XBAPI VOID NTAPI RtlMoveMemory
(
    PVOID Destination,
    CONST PVOID Source,
    ULONG Length
);

XBAPI NTSTATUS NTAPI RtlMultiByteToUnicodeN
(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

XBAPI NTSTATUS NTAPI RtlMultiByteToUnicodeSize
(
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

XBAPI ULONG NTAPI RtlNtStatusToDosError
(
    IN NTSTATUS Status
);

XBAPI VOID NTAPI RtlRaiseException
(
    IN PEXCEPTION_RECORD ExceptionRecord
);

XBAPI VOID NTAPI RtlRaiseStatus
(
    IN NTSTATUS Status
);

XBAPI VOID NTAPI RtlRip
(
    IN PVOID ApiName,
    IN PVOID Expression,
    IN PVOID Message
);

XBAPI VOID CDECL RtlSnprintf
(
    CHAR *,
    SIZE_T,
    CONST CHAR *,
    ...
);

XBAPI VOID CDECL RtlSprintf
(
    CHAR *,
    CONST CHAR *,
    ...
);

XBAPI BOOLEAN NTAPI RtlTimeFieldsToTime
(
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
);

XBAPI VOID NTAPI RtlTimeToTimeFields
(
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
);

XBAPI BOOLEAN NTAPI RtlTryEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI ULONG FASTCALL RtlUlongByteSwap
(
    IN ULONG Source
);

XBAPI NTSTATUS NTAPI RtlUnicodeStringToAnsiString
(
    OUT PSTRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS NTAPI RtlUnicodeStringToInteger
(
    PUNICODE_STRING String,
    ULONG Base,
    PULONG Value
);

XBAPI NTSTATUS NTAPI RtlUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI NTSTATUS NTAPI RtlUnicodeToMultiByteSize
(
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI VOID NTAPI RtlUnwind
(
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
);

XBAPI WCHAR NTAPI RtlUpcaseUnicodeChar
(
    WCHAR SourceCharacter
);

XBAPI NTSTATUS NTAPI RtlUpcaseUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS NTAPI RtlUpcaseUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI CHAR NTAPI RtlUpperChar
(
    CHAR Character
);

XBAPI VOID NTAPI RtlUpperString
(
    PSTRING DestinationString,
    PSTRING SourceString
);

XBAPI USHORT FASTCALL RtlUshortByteSwap
(
    IN USHORT Source
);

XBAPI VOID CDECL RtlVsnprintf
(
    CHAR *,
    SIZE_T,
    CONST CHAR*,
    ...
);

XBAPI VOID CDECL RtlVsprintf
(
    CHAR *,
    CONST CHAR*,
    ...
);

XBAPI ULONG NTAPI RtlWalkFrameChain
(
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
);

XBAPI VOID NTAPI RtlZeroMemory
(
    IN VOID UNALIGNED *Destination,
    IN SIZE_T Length
);

#endif
