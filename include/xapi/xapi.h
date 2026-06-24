#ifndef RXDK_XAPI_H
#define RXDK_XAPI_H

/*
 * RXDK libxapi umbrella (Xbox-only).
 *
 * Kernel types/APIs from xboxkrnl (replaces sdk/nt.h). XDK user surface via xtl
 * in xapip.h — windef skips winnt.h when NT_INCLUDED is set (leak xAPI pattern).
 */

#ifndef _XBOX
#define _XBOX 1
#endif
#define XBOX 1

#ifndef _X86_
#define _X86_ 1
#endif
#ifndef i386
#define i386 1
#endif
#ifndef _M_IX86
#define _M_IX86 600
#endif

#define NT_UP 1
#define STD_CALL 1
#ifndef _XAPI_
#define _XAPI_ 1
#endif

#define _KERNEL32_
#define _USER32_
#define _GDI32_
#define _WINMM_

#ifndef NTKERNELAPI
#define NTKERNELAPI
#endif
#ifndef NTHALAPI
#define NTHALAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <xboxkrnl/xboxkrnl.h>

#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

#ifndef NtGetTickCount
#define NtGetTickCount() ((DWORD)KeTickCount)
#endif

#include <xapi/xdk_bridge.h>
#include <xapi/kpcrb_bridge.h>
#include <xapi/pe32.h>

#ifndef PCOBJECT_STRING
typedef POBJECT_STRING PCOBJECT_STRING;
#endif
#ifndef PCOSTR
typedef PCSTR PCOSTR;
#endif
typedef OBJECT_STRING COBJECT_STRING;
#define OTEXT(quote) quote
#ifndef OBJECT_NULL
#define OBJECT_NULL ((OCHAR)0)
#endif
#ifndef STATUS_UNRECOGNIZED_VOLUME
#define STATUS_UNRECOGNIZED_VOLUME ((NTSTATUS)0xC000014FL)
#endif
#ifndef STATUS_UNRECOGNIZED_MEDIA
#define STATUS_UNRECOGNIZED_MEDIA ((NTSTATUS)0xC0000014L)
#endif
#ifndef STATUS_NOT_SAME_DEVICE
#define STATUS_NOT_SAME_DEVICE ((NTSTATUS)0xC00000D4L)
#endif
#ifndef STATUS_NOT_A_DIRECTORY
#define STATUS_NOT_A_DIRECTORY ((NTSTATUS)0xC0000103L)
#endif
#ifndef STATUS_DISK_FULL
#define STATUS_DISK_FULL ((NTSTATUS)0xC000007FL)
#endif
#ifndef STATUS_REQUEST_ABORTED
#define STATUS_REQUEST_ABORTED ((NTSTATUS)0xC0000240L)
#endif
#ifndef STATUS_TIMER_RESUME_IGNORED
#define STATUS_TIMER_RESUME_IGNORED ((NTSTATUS)0xC0000250L)
#endif
#ifndef STATUS_NO_SUCH_FILE
#define STATUS_NO_SUCH_FILE ((NTSTATUS)0xC000000FL)
#endif
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif
int sprintf(char *str, const char *format, ...);
#define soprintf sprintf

#ifdef __cplusplus
}
#endif

#endif /* RXDK_XAPI_H */
