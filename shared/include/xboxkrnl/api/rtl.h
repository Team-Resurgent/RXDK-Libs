#ifndef XBOXKRNL_API_RTL_H
#define XBOXKRNL_API_RTL_H

XBAPI NTSTATUS STDCALL RtlAnsiStringToUnicodeString
(
    PUNICODE_STRING DestinationString,
    PSTRING SourceString,
    BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS STDCALL RtlAppendStringToString
(
    IN PSTRING Destination,
    IN PSTRING Source
);

XBAPI NTSTATUS STDCALL RtlAppendUnicodeStringToString
(
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source
);

XBAPI NTSTATUS STDCALL RtlAppendUnicodeToString
(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

XBAPI VOID STDCALL RtlAssert
(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
);

XBAPI VOID STDCALL RtlCaptureContext
(
    OUT PCONTEXT ContextRecord
);

XBAPI USHORT STDCALL RtlCaptureStackBackTrace
(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
);

XBAPI NTSTATUS STDCALL RtlCharToInteger
(
    IN PCSZ String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value
);

XBAPI SIZE_T STDCALL RtlCompareMemory
(
    IN CONST VOID *Source1,
    IN CONST VOID *Source2,
    IN SIZE_T Length
);

XBAPI SIZE_T STDCALL RtlCompareMemoryUlong
(
    PVOID Source,
    SIZE_T Length,
    ULONG Pattern
);

XBAPI LONG STDCALL RtlCompareString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI LONG STDCALL RtlCompareUnicodeString
(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
);

XBAPI VOID STDCALL RtlCopyString
(
    OUT PSTRING DestinationString,
    IN PSTRING SourceString
);

XBAPI VOID STDCALL RtlCopyUnicodeString
(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString
);

XBAPI BOOLEAN STDCALL RtlCreateUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
);

XBAPI WCHAR STDCALL RtlDowncaseUnicodeChar
(
    WCHAR SourceCharacter
);

XBAPI NTSTATUS STDCALL RtlDowncaseUnicodeString
(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

XBAPI VOID STDCALL RtlEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID STDCALL RtlEnterCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI BOOLEAN STDCALL RtlEqualString
(
    IN CONST PSTRING String1,
    IN CONST PSTRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI BOOLEAN STDCALL RtlEqualUnicodeString
(
    IN CONST PUNICODE_STRING String1,
    IN CONST PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
);

XBAPI LARGE_INTEGER STDCALL RtlExtendedIntegerMultiply
(
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
);

XBAPI LARGE_INTEGER STDCALL RtlExtendedLargeIntegerDivide
(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
);

XBAPI LARGE_INTEGER STDCALL RtlExtendedMagicDivide
(
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
);

XBAPI VOID STDCALL RtlFillMemory
(
    PVOID Destination,
    ULONG Length,
    UCHAR Fill
);

XBAPI VOID STDCALL RtlFillMemoryUlong
(
    PVOID Destination,
    SIZE_T Length,
    ULONG Pattern
);

XBAPI VOID STDCALL RtlFreeAnsiString
(
    PANSI_STRING AnsiString
);

XBAPI VOID STDCALL RtlFreeUnicodeString
(
    PUNICODE_STRING UnicodeString
);

XBAPI VOID STDCALL RtlGetCallersAddress
(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
);

XBAPI VOID STDCALL RtlInitAnsiString
(
    PANSI_STRING DestinationString,
    IN PCSZ SourceString
);

XBAPI VOID STDCALL RtlInitializeCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID STDCALL RtlInitUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

XBAPI NTSTATUS STDCALL RtlIntegerToChar
(
    ULONG Value,
    ULONG Base,
    LONG OutputLength,
    PSZ String
);

XBAPI NTSTATUS STDCALL RtlIntegerToUnicodeString
(
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String
);

XBAPI VOID STDCALL RtlLeaveCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI VOID STDCALL RtlLeaveCriticalSectionAndRegion
(
    PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI CHAR STDCALL RtlLowerChar
(
    IN CHAR Character
);

XBAPI VOID STDCALL RtlMapGenericMask
(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

XBAPI VOID STDCALL RtlMoveMemory
(
    PVOID Destination,
    CONST PVOID Source,
    ULONG Length
);

XBAPI NTSTATUS STDCALL RtlMultiByteToUnicodeN
(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

XBAPI NTSTATUS STDCALL RtlMultiByteToUnicodeSize
(
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
);

XBAPI ULONG STDCALL RtlNtStatusToDosError
(
    IN NTSTATUS Status
);

XBAPI VOID STDCALL RtlRaiseException
(
    IN PEXCEPTION_RECORD ExceptionRecord
);

XBAPI VOID STDCALL RtlRaiseStatus
(
    IN NTSTATUS Status
);

XBAPI VOID STDCALL RtlRip
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

XBAPI BOOLEAN STDCALL RtlTimeFieldsToTime
(
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
);

XBAPI VOID STDCALL RtlTimeToTimeFields
(
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
);

XBAPI BOOLEAN STDCALL RtlTryEnterCriticalSection
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

XBAPI ULONG FASTCALL RtlUlongByteSwap
(
    IN ULONG Source
);

XBAPI NTSTATUS STDCALL RtlUnicodeStringToAnsiString
(
    OUT PSTRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS STDCALL RtlUnicodeStringToInteger
(
    PUNICODE_STRING String,
    ULONG Base,
    PULONG Value
);

XBAPI NTSTATUS STDCALL RtlUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI NTSTATUS STDCALL RtlUnicodeToMultiByteSize
(
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI VOID STDCALL RtlUnwind
(
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
);

XBAPI WCHAR STDCALL RtlUpcaseUnicodeChar
(
    WCHAR SourceCharacter
);

XBAPI NTSTATUS STDCALL RtlUpcaseUnicodeString
(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

XBAPI NTSTATUS STDCALL RtlUpcaseUnicodeToMultiByteN
(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
);

XBAPI CHAR STDCALL RtlUpperChar
(
    CHAR Character
);

XBAPI VOID STDCALL RtlUpperString
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

XBAPI ULONG STDCALL RtlWalkFrameChain
(
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
);

XBAPI VOID STDCALL RtlZeroMemory
(
    IN VOID UNALIGNED *Destination,
    IN SIZE_T Length
);

#endif
