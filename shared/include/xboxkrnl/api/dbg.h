#ifndef XBOXKRNL_API_DBG_H
#define XBOXKRNL_API_DBG_H

VOID STDCALL DbgBreakPoint (void);

VOID STDCALL DbgBreakPointWithStatus
(
    IN ULONG Status
);

VOID STDCALL DbgLoadImageSymbols
(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId
);

ULONG CDECL DbgPrint
(
    PCSTR Format,
    ...
);

ULONG STDCALL DbgPrompt
(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength
);

VOID STDCALL DbgUnLoadImageSymbols
(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId
);

#endif
