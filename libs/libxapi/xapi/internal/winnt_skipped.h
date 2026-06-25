#ifndef RXDK_XAPI_WINNT_SKIPPED_H
#define RXDK_XAPI_WINNT_SKIPPED_H

/*
 * Typedefs windef.h normally pulls from winnt.h when NT_INCLUDED is set.
 * For USB/kernel TUs that use sdk/nt.h instead of xboxkrnl — no xboxdef.h.
 */

#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer) \
    ((ULONG_PTR)(ArgumentPointer) != (ULONG_PTR)(NULL))
#endif

#ifndef Int32x32To64
#define Int32x32To64(a, b) ((LONGLONG)((LONGLONG)(LONG)(a) * (LONG)(b)))
#endif
#ifndef UInt32x32To64
#define UInt32x32To64(a, b) ((ULONGLONG)((ULONGLONG)(DWORD)(a) * (DWORD)(b)))
#endif

#ifndef RtlInitObjectString
#define RtlInitObjectString RtlInitAnsiString
#endif

#endif /* RXDK_XAPI_WINNT_SKIPPED_H */
