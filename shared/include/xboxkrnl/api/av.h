#ifndef XBOXKRNL_API_AV_H
#define XBOXKRNL_API_AV_H

XBAPI PVOID STDCALL AvGetSavedDataAddress(void);

XBAPI VOID STDCALL AvSendTVEncoderOption
(
    IN PVOID RegisterBase,
    IN ULONG Option,
    IN ULONG Param,
    OUT PULONG Result
);

XBAPI ULONG STDCALL AvSetDisplayMode
(
    IN PVOID RegisterBase,
    IN ULONG Step,
    IN ULONG DisplayMode,
    IN ULONG SourceColorFormat,
    IN ULONG Pitch,
    IN ULONG FrameBuffer
);

XBAPI VOID STDCALL AvSetSavedDataAddress
(
    IN PVOID Address
);

#endif
