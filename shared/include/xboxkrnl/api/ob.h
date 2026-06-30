#ifndef XBOXKRNL_API_OB_H
#define XBOXKRNL_API_OB_H

NTSTATUS STDCALL ObCreateObject
(
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectBodySize,
    OUT PVOID *Object
);

XBAPI OBJECT_TYPE ObDirectoryObjectType[1];

VOID FASTCALL ObfDereferenceObject
(
    IN PVOID Object
);

VOID FASTCALL ObfReferenceObject
(
    IN PVOID Object
);

NTSTATUS STDCALL ObInsertObject
(
    IN PVOID Object,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PHANDLE Handle
);

VOID STDCALL ObMakeTemporaryObject
(
    IN PVOID Object
);

NTSTATUS STDCALL ObOpenObjectByName
(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
);

NTSTATUS STDCALL ObOpenObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    OUT PHANDLE Handle
);

XBAPI OBJECT_HANDLE_TABLE ObpObjectHandleTable;

NTSTATUS STDCALL ObReferenceObjectByHandle
(
    IN HANDLE Handle,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    OUT PVOID *ReturnedObject
);

NTSTATUS STDCALL ObReferenceObjectByName
(
    IN POBJECT_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
);

NTSTATUS STDCALL ObReferenceObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType
);

XBAPI OBJECT_TYPE ObSymbolicLinkObjectType[1];

#endif
