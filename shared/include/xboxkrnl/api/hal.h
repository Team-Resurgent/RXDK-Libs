#ifndef XBOXKRNL_API_HAL_H
#define XBOXKRNL_API_HAL_H

XBAPI DWORD HalBootSMCVideoMode;

XBAPI VOID FASTCALL HalClearSoftwareInterrupt
(
    IN KIRQL RequestIrql
);

XBAPI VOID STDCALL HalDisableSystemInterrupt
(
    IN ULONG BusInterruptLevel
);

XBAPI ULONG HalDiskCachePartitionCount;

XBAPI STRING HalDiskModelNumber;

XBAPI STRING HalDiskSerialNumber;

XBAPI VOID STDCALL HalEnableSecureTrayEject(void);

XBAPI VOID STDCALL HalEnableSystemInterrupt
(
    IN ULONG BusInterruptLevel,
    IN KINTERRUPT_MODE InterruptMode
);

XBAPI ULONG STDCALL HalGetInterruptVector
(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql
);

XBAPI VOID STDCALL HalInitiateShutdown(void);

XBAPI BOOLEAN STDCALL HalIsResetOrShutdownPending(void);

XBAPI NTSTATUS STDCALL HalReadSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN ReadWordValue,
    OUT ULONG *DataValue
);

XBAPI NTSTATUS STDCALL HalReadSMCTrayState
(
    OUT PULONG TrayState,
    OUT PULONG TrayStateChangeCount OPTIONAL
);

XBAPI VOID STDCALL HalReadWritePCISpace
(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG RegisterNumber,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WritePCISpace
);

XBAPI VOID STDCALL HalRegisterShutdownNotification
(
    IN PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
    IN BOOLEAN Register
);

XBAPI VOID FASTCALL HalRequestSoftwareInterrupt
(
    KIRQL RequestIrql
);

XBAPI VOID DECLSPEC_NORETURN STDCALL HalReturnToFirmware
(
    IN FIRMWARE_REENTRY Routine
);

XBAPI NTSTATUS STDCALL HalWriteSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN WriteWordValue,
    IN ULONG DataValue
);

XBAPI NTSTATUS STDCALL HalWriteSMCScratchRegister
(
    IN ULONG ScratchRegister
);

XBAPI VOID STDCALL WRITE_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
);

XBAPI VOID STDCALL WRITE_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
);

XBAPI VOID STDCALL WRITE_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
);

#endif
