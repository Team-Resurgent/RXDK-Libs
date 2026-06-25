#ifndef RXDK_XAPI_NTURTL_H
#define RXDK_XAPI_NTURTL_H

/*
 * Shadow of leak nturtl.h for libxapi (xAPI runtime, not user-mode NT).
 * Provides types/macros ntrtlp.h and heap.c expect without sdk/nt.h.
 */

#ifndef _NTURTL_
#define _NTURTL_

#include "nt_bridge.h"

#ifndef NTURTLAPI
#define NTURTLAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RTL_RELATIVE_NAME {
    STRING RelativeName;
    HANDLE ContainingDirectory;
} RTL_RELATIVE_NAME, *PRTL_RELATIVE_NAME;

typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,
    RtlPathTypeDriveAbsolute,
    RtlPathTypeDriveRelative,
    RtlPathTypeRooted,
    RtlPathTypeRelative,
    RtlPathTypeLocalDevice,
    RtlPathTypeRootLocalDevice,
} RTL_PATH_TYPE;

typedef NTSTATUS (__stdcall *PRTL_HEAP_COMMIT_ROUTINE)(
    IN PVOID Base,
    IN OUT PVOID *CommitAddress,
    IN OUT PSIZE_T CommitSize
);

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_GENERATE_EXCEPTIONS        0x00000004
#define HEAP_ZERO_MEMORY                0x00000008
#define HEAP_CLASS_1                    0x00001000

#ifdef __cplusplus
}
#endif

#endif /* _NTURTL_ */
#endif /* RXDK_XAPI_NTURTL_H */
