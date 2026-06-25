#pragma once
#define RXDK_USB_BRIDGE_H

/* USB driver slices run in the kernel runtime (was -DNTOS_KERNEL_RUNTIME). */
#ifndef NTOS_KERNEL_RUNTIME
#define NTOS_KERNEL_RUNTIME 1
#endif



/*
 * Force-included for libxapi USB slices (ohcd, usbd, hub, mu, xid).
 *
 * Vendor ntos headers mark kernel entry points DECLSPEC_IMPORT when building
 * as a user-mode driver (!_NTSYSTEM_). libxapi links xboxkrnl.lib directly;
 * RXDK_USB_LINK clears NTKERNELAPI in ntosdef.h.
 *
 * RXDK_USB_LINK: kernel DATA imports use rxdk_kernel_import_ptrs.h (struct-in-IAT fix).
 */

#define RXDK_USB_LINK 1

/*
 * lld-link does not guarantee a contiguous .XPP$Class* table the way MS LINK
 * does; walk an explicit class-driver list in usbd.cpp instead.
 */
#define RXDK_USB_CLASS_TABLE 1

/* Set to 1 only when debugging USB init on kit (verbose DbgPrint). */
#define RXDK_USB_TRACE 0

#if RXDK_USB_TRACE
#ifdef __cplusplus
extern "C" {
#endif
unsigned long DbgPrint(const char *Format, ...);
#ifdef __cplusplus
}
#endif
#define RXDK_USB_TRACE_MSG(msg) DbgPrint("rxdk-usb: " msg "\n")
#define RXDK_USB_TRACE_MSG1(msg, a) DbgPrint("rxdk-usb: " msg "\n", (a))
#define RXDK_USB_TRACE_MSG2(msg, a, b) DbgPrint("rxdk-usb: " msg "\n", (a), (b))
#else
#define RXDK_USB_TRACE_MSG(msg) ((void)0)
#define RXDK_USB_TRACE_MSG1(msg, a) ((void)0)
#define RXDK_USB_TRACE_MSG2(msg, a, b) ((void)0)
#endif

#ifdef DECLSPEC_IMPORT
#undef DECLSPEC_IMPORT
#endif
#define DECLSPEC_IMPORT

#ifdef NTKERNELAPI
#undef NTKERNELAPI
#endif
#define NTKERNELAPI

#ifdef NTHALAPI
#undef NTHALAPI
#endif
#define NTHALAPI

/*
 * xboxkrnl NTAPI entry points use x86 __stdcall. Vendor USB sources spell that
 * explicitly on kernel callbacks (ISR/DPC/HAL); do not rely on empty NTAPI/__attribute__((__stdcall__))
 * from windef.h — see build/generated/xapi_site.h (libxapi) / xapi_title_site.h.
 */

#ifndef InitializeListHead
#define InitializeListHead(ListHead) ((ListHead)->Flink = (ListHead)->Blink = (ListHead))
#endif

#include <string.h>
#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#endif
#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

