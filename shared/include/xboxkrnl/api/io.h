#ifndef XBOXKRNL_API_IO_H
#define XBOXKRNL_API_IO_H

XBAPI PFN_COUNT STDCALL FscGetCacheSize (void);

XBAPI VOID STDCALL FscInvalidateIdleBlocks (void);

XBAPI NTSTATUS STDCALL FscSetCacheSize
(
    IN PFN_COUNT NumberOfCachePages
);

XBAPI IDE_CHANNEL_OBJECT IdexChannelObject;

XBAPI PIRP STDCALL IoAllocateIrp
(
    IN CCHAR StackSize
);

XBAPI PIRP STDCALL IoBuildAsynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
);

XBAPI PIRP STDCALL IoBuildDeviceIoControlRequest
(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

XBAPI PIRP STDCALL IoBuildSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

XBAPI NTSTATUS STDCALL IoCheckShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
);

XBAPI OBJECT_TYPE IoCompletionObjectType[1];

XBAPI NTSTATUS STDCALL IoCreateDevice
(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN POBJECT_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
);

XBAPI NTSTATUS STDCALL IoCreateFile
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN ULONG Options
);

XBAPI NTSTATUS STDCALL IoCreateSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName,
    IN POBJECT_STRING DeviceName
);

XBAPI VOID STDCALL IoDeleteDevice
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI NTSTATUS STDCALL IoDeleteSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName
);

XBAPI OBJECT_TYPE IoDeviceObjectType[1];

XBAPI NTSTATUS STDCALL IoDismountVolume
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI NTSTATUS STDCALL IoDismountVolumeByName
(
    IN POBJECT_STRING DeviceName
);

XBAPI NTSTATUS FASTCALL IofCallDriver
(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

XBAPI VOID FASTCALL IofCompleteRequest
(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
);

XBAPI OBJECT_TYPE IoFileObjectType[1];

XBAPI VOID STDCALL IoFreeIrp
(
    IN PIRP Irp
);

XBAPI VOID STDCALL IoInitializeIrp
(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
);

XBAPI NTSTATUS STDCALL IoInvalidDeviceRequest
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

XBAPI VOID STDCALL IoMarkIrpMustComplete
(
    IN OUT PIRP Irp
);

XBAPI NTSTATUS STDCALL IoQueryFileInformation
(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
);

XBAPI NTSTATUS STDCALL IoQueryVolumeInformation
(
    IN PFILE_OBJECT FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    OUT PVOID FsInformation,
    OUT PULONG ReturnedLength
);

XBAPI VOID STDCALL IoQueueThreadIrp
(
    IN PIRP Irp
);

XBAPI VOID STDCALL IoRemoveShareAccess
(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
);

XBAPI NTSTATUS STDCALL IoSetIoCompletion
(
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation
);

XBAPI VOID STDCALL IoSetShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
);

XBAPI VOID STDCALL IoStartNextPacket
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI VOID STDCALL IoStartNextPacketByKey
(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Key
);

XBAPI VOID STDCALL IoStartPacket
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL
);

XBAPI NTSTATUS STDCALL IoSynchronousDeviceIoControlRequest
(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnedOutputBufferLength OPTIONAL,
    IN BOOLEAN InternalDeviceIoControl
);

XBAPI NTSTATUS STDCALL IoSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL
);

#endif
