#ifndef XBOXKRNL_API_DBG_H
#define XBOXKRNL_API_DBG_H

XBAPI VOID STDCALL DbgBreakPoint (void);

XBAPI VOID STDCALL DbgBreakPointWithStatus
(
    IN ULONG Status
);

XBAPI VOID STDCALL DbgLoadImageSymbols
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

XBAPI ULONG STDCALL DbgPrompt
(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength
);

XBAPI VOID STDCALL DbgUnLoadImageSymbols
(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId
);

#endif
