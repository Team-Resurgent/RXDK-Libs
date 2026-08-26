/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * DMusBuff.h -- the buffer format for DirectMusic events. Defines the per-event
 * header (delta time, position and length) that precedes each event payload in
 * a message buffer. Shared between the user-mode and kernel-mode components.
 */

#ifndef _DMusBuff_
#define _DMusBuff_

/* Format of DirectMusic events in a buffer
 *
 * A buffer contains 1 or more events, each with the following header.
 * Immediately following the header is the event data. The header+data
 * size is rounded to the nearest quadword (8 bytes).
 */
 
#include <pshpack4.h>                       /* Do not pad at end - that's where the data is */ 
typedef struct _DMUS_EVENTHEADER *LPDMUS_EVENTHEADER;
typedef struct _DMUS_EVENTHEADER
{
    DWORD           cbEvent;                /* Unrounded bytes in event */
    DWORD           dwChannelGroup;         /* Channel group of event */
    REFERENCE_TIME  rtDelta;                /* Delta from start time of entire buffer */
    DWORD           dwFlags;                /* Flags DMUS_EVENT_xxx */
} DMUS_EVENTHEADER;
#include <poppack.h>

#define DMUS_EVENT_STRUCTURED   0x00000001  /* Unstructured data (SysEx, etc.) */

/* The number of bytes to allocate for an event with 'cb' data bytes.
 */ 
#define QWORD_ALIGN(x) (((x) + 7) & ~7)
#define DMUS_EVENT_SIZE(cb) QWORD_ALIGN(sizeof(DMUS_EVENTHEADER) + cb)


#endif /* _DMusBuff_ */


