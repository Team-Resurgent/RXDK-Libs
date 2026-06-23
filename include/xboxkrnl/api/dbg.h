#ifndef XBOXKRNL_API_DBG_H
#define XBOXKRNL_API_DBG_H

XBAPI VOID NTAPI DbgBreakPoint (void);

XBAPI VOID NTAPI DbgBreakPointWithStatus
(
    IN ULONG Status
);

XBAPI VOID NTAPI DbgLoadImageSymbols
(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId
);

XBAPI ULONG CDECL DbgPrint
(
    PCSTR Format,
    ...
);

XBAPI ULONG NTAPI DbgPrompt
(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength
);

XBAPI VOID NTAPI DbgUnLoadImageSymbols
(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId
);

#endif
