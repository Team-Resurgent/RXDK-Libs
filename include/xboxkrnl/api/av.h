#ifndef XBOXKRNL_API_AV_H
#define XBOXKRNL_API_AV_H

XBAPI PVOID NTAPI AvGetSavedDataAddress(void);

XBAPI VOID NTAPI AvSendTVEncoderOption
(
    IN PVOID RegisterBase,
    IN ULONG Option,
    IN ULONG Param,
    OUT PULONG Result
);

XBAPI ULONG NTAPI AvSetDisplayMode
(
    IN PVOID RegisterBase,
    IN ULONG Step,
    IN ULONG DisplayMode,
    IN ULONG SourceColorFormat,
    IN ULONG Pitch,
    IN ULONG FrameBuffer
);

XBAPI VOID NTAPI AvSetSavedDataAddress
(
    IN PVOID Address
);

#endif
