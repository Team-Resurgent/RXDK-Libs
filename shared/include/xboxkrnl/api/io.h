#ifndef XBOXKRNL_API_IO_H
#define XBOXKRNL_API_IO_H

PFN_COUNT STDCALL FscGetCacheSize (void);

VOID STDCALL FscInvalidateIdleBlocks (void);

NTSTATUS STDCALL FscSetCacheSize
(
    IN PFN_COUNT NumberOfCachePages
);

XBAPI IDE_CHANNEL_OBJECT IdexChannelObject;

PIRP STDCALL IoAllocateIrp
(
    IN CCHAR StackSize
);

PIRP STDCALL IoBuildAsynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
);

PIRP STDCALL IoBuildDeviceIoControlRequest
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

PIRP STDCALL IoBuildSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS STDCALL IoCheckShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
);

XBAPI OBJECT_TYPE IoCompletionObjectType[1];

NTSTATUS STDCALL IoCreateDevice
(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN POBJECT_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
);

NTSTATUS STDCALL IoCreateFile
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

NTSTATUS STDCALL IoCreateSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName,
    IN POBJECT_STRING DeviceName
);

VOID STDCALL IoDeleteDevice
(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS STDCALL IoDeleteSymbolicLink
(
    IN POBJECT_STRING SymbolicLinkName
);

XBAPI OBJECT_TYPE IoDeviceObjectType[1];

NTSTATUS STDCALL IoDismountVolume
(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS STDCALL IoDismountVolumeByName
(
    IN POBJECT_STRING DeviceName
);

NTSTATUS FASTCALL IofCallDriver
(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

VOID FASTCALL IofCompleteRequest
(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
);

XBAPI OBJECT_TYPE IoFileObjectType[1];

VOID STDCALL IoFreeIrp
(
    IN PIRP Irp
);

VOID STDCALL IoInitializeIrp
(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
);

NTSTATUS STDCALL IoInvalidDeviceRequest
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID STDCALL IoMarkIrpMustComplete
(
    IN OUT PIRP Irp
);

NTSTATUS STDCALL IoQueryFileInformation
(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
);

NTSTATUS STDCALL IoQueryVolumeInformation
(
    IN PFILE_OBJECT FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    OUT PVOID FsInformation,
    OUT PULONG ReturnedLength
);

VOID STDCALL IoQueueThreadIrp
(
    IN PIRP Irp
);

VOID STDCALL IoRemoveShareAccess
(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
);

NTSTATUS STDCALL IoSetIoCompletion
(
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation
);

VOID STDCALL IoSetShareAccess
(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
);

VOID STDCALL IoStartNextPacket
(
    IN PDEVICE_OBJECT DeviceObject
);

VOID STDCALL IoStartNextPacketByKey
(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Key
);

VOID STDCALL IoStartPacket
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL
);

NTSTATUS STDCALL IoSynchronousDeviceIoControlRequest
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

NTSTATUS STDCALL IoSynchronousFsdRequest
(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL
);

#endif
