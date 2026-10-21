/*
 * Team Resurgent 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Fast copies for the Pentium III (RXDK extension; libs/libc/xbox/sse1_memcpy.c).
 *
 * memcpy is picolibc's integer copy (rep movs): safe anywhere, including DPCs
 * and interrupt service routines. These use SSE (xmm registers), which the
 * kernel does not save for code that interrupts a thread, so call them only
 * at PASSIVE_LEVEL -- from threads, never from a DPC, an ISR or a kernel APC.
 *
 *   memcpy_fast     cached stores for the sizes a title copies every frame,
 *                   streaming (non-temporal) stores from 128 KB up
 *   memcpy_fast_nt  streaming stores for every copy over 64 bytes: for
 *                   destinations the CPU will not read back soon (vertex,
 *                   texture and framebuffer uploads)
 *
 * Both are memcpy, not memmove: the ranges must not overlap.
 */
#ifndef RXDK_MEMCPY_FAST_H
#define RXDK_MEMCPY_FAST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy_fast(void* dst, const void* src, size_t n);
void* memcpy_fast_nt(void* dst, const void* src, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* RXDK_MEMCPY_FAST_H */
