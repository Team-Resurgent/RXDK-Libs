#ifndef XBOXKRNL_API_OB_H
#define XBOXKRNL_API_OB_H

XBAPI NTSTATUS NTAPI ObCreateObject
(
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectBodySize,
    OUT PVOID *Object
);

XBAPI OBJECT_TYPE ObDirectoryObjectType;

XBAPI VOID FASTCALL ObfDereferenceObject
(
    IN PVOID Object
);

XBAPI VOID FASTCALL ObfReferenceObject
(
    IN PVOID Object
);

XBAPI NTSTATUS NTAPI ObInsertObject
(
    IN PVOID Object,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PHANDLE Handle
);

XBAPI VOID NTAPI ObMakeTemporaryObject
(
    IN PVOID Object
);

XBAPI NTSTATUS NTAPI ObOpenObjectByName
(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
);

XBAPI NTSTATUS NTAPI ObOpenObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    OUT PHANDLE Handle
);

XBAPI OBJECT_HANDLE_TABLE ObpObjectHandleTable;

XBAPI NTSTATUS NTAPI ObReferenceObjectByHandle
(
    IN HANDLE Handle,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    OUT PVOID *ReturnedObject
);

XBAPI NTSTATUS NTAPI ObReferenceObjectByName
(
    IN POBJECT_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
);

XBAPI NTSTATUS NTAPI ObReferenceObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType
);

XBAPI OBJECT_TYPE ObSymbolicLinkObjectType;

#endif
