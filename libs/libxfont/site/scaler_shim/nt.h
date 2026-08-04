/*
 * RXDK shim for the TrueType scan converter.
 *
 * scaler/fsconfig.h pulls <nt.h> and <ntrtl.h> under FSCFG_INTERNAL for exactly
 * two things: RtlZeroMemory and RtlCopyMemory, which its MEMSET/MEMCPY macros
 * wrap. The scaler builds in its own slice precisely because it declares its own
 * ULONG/LPSTR/LARGE_INTEGER and cannot see libxapi's NT headers without those
 * colliding -- so supply just the two routines instead of the header set.
 *
 * (ntrtl.h is a sibling shim that includes this one, so either include order in
 * fsconfig.h resolves.)
 */
#ifndef RXDK_XFONT_SCALER_NT_SHIM_H
#define RXDK_XFONT_SCALER_NT_SHIM_H

#include <string.h>

/* fsconfig.h defines ClientIDType as ULONG_PTR ("to get ready for the 64 bits
 * platform"), which normally arrives with the NT headers. Pointer-sized on the
 * Xbox's 32-bit x86. Without it ClientIDType expands to an unknown identifier
 * and every callback typedef using it parses as a K&R parameter list. */
#ifndef _ULONG_PTR_DEFINED
#define _ULONG_PTR_DEFINED
typedef unsigned long ULONG_PTR;
typedef long          LONG_PTR;
typedef unsigned long UINT_PTR;
typedef long          INT_PTR;
#endif

/* fsconfig.h's MEMSET drops the fill value -- see the comment there: "in all
 * uses in the rasterizer MEMSET is used to zero out the mem". */
#define RtlZeroMemory(dst, size)      memset((dst), 0, (size))
#define RtlCopyMemory(dst, src, size) memcpy((dst), (src), (size))

#endif
