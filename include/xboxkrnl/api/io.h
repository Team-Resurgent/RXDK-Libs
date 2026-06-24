#ifndef XBOXKRNL_API_IO_H
#define XBOXKRNL_API_IO_H

XBAPI PFN_COUNT NTAPI FscGetCacheSize (void);

XBAPI VOID NTAPI FscInvalidateIdleBlocks (void);

XBAPI NTSTATUS NTAPI FscSetCacheSize
(
    IN PFN_COUNT NumberOfCachePages
);

XBAPI IDE_CHANNEL_OBJECT IdexChannelObject;

XBAPI PIRP NTAPI IoAllocateIrp
(
    IN CCHAR StackSize
);

XBAPI PIRP NTAPI IoBuildAsynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
);

XBAPI PIRP NTAPI IoBuildDeviceIoControlRequest
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

XBAPI PIRP NTAPI IoBuildSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

XBAPI NTSTATUS NTAPI IoCheckShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
);

RXDK_XBOXKRNL_OBJECT_TYPE(IoCompletionObjectType);

XBAPI NTSTATUS NTAPI IoCreateDevice
(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN POBJECT_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
);

XBAPI NTSTATUS NTAPI IoCreateFile
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

XBAPI NTSTATUS NTAPI IoCreateSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName,
    IN POBJECT_STRING DeviceName
);

XBAPI VOID NTAPI IoDeleteDevice
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI NTSTATUS NTAPI IoDeleteSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName
);

RXDK_XBOXKRNL_OBJECT_TYPE(IoDeviceObjectType);

XBAPI NTSTATUS NTAPI IoDismountVolume
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI NTSTATUS NTAPI IoDismountVolumeByName
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

RXDK_XBOXKRNL_OBJECT_TYPE(IoFileObjectType);

XBAPI VOID NTAPI IoFreeIrp
(
    IN PIRP Irp
);

XBAPI VOID NTAPI IoInitializeIrp
(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
);

XBAPI NTSTATUS NTAPI IoInvalidDeviceRequest
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

XBAPI VOID NTAPI IoMarkIrpMustComplete
(
    IN OUT PIRP Irp
);

XBAPI NTSTATUS NTAPI IoQueryFileInformation
(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
);

XBAPI NTSTATUS NTAPI IoQueryVolumeInformation
(
    IN PFILE_OBJECT FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    OUT PVOID FsInformation,
    OUT PULONG ReturnedLength
);

XBAPI VOID NTAPI IoQueueThreadIrp
(
    IN PIRP Irp
);

XBAPI VOID NTAPI IoRemoveShareAccess
(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
);

XBAPI NTSTATUS NTAPI IoSetIoCompletion
(
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation
);

XBAPI VOID NTAPI IoSetShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
);

XBAPI VOID NTAPI IoStartNextPacket
(
    IN PDEVICE_OBJECT DeviceObject
);

XBAPI VOID NTAPI IoStartNextPacketByKey
(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Key
);

XBAPI VOID NTAPI IoStartPacket
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL
);

XBAPI NTSTATUS NTAPI IoSynchronousDeviceIoControlRequest
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

XBAPI NTSTATUS NTAPI IoSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL
);

#endif
