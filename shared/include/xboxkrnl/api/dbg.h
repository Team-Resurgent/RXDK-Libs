/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Dbg* kernel imports: the debug channel to a host debugger. Provides breakpoint
 * traps, formatted debug-string output, an interactive prompt, and the notify
 * hooks that tell the debugger when image symbols load or unload. On a retail
 * console with no debugger attached these are largely inert.
 */

#ifndef XBOXKRNL_API_DBG_H
#define XBOXKRNL_API_DBG_H

/* Executes a breakpoint trap, breaking into the attached debugger. */
VOID STDCALL DbgBreakPoint(void);

/* Like DbgBreakPoint, but passes Status to the debugger as the break reason. */
VOID STDCALL DbgBreakPointWithStatus(
    IN ULONG Status);

/* Notifies the debugger that an image's symbols are available. FileName names
 * the image, ImageBase is its load address, ProcessId identifies the owner. */
VOID STDCALL DbgLoadImageSymbols(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId);

/* printf-style output to the debug channel. Returns the number of characters
 * emitted. No-op sink when no debugger is listening. */
ULONG CDECL DbgPrint(
    PCSTR Format,
    ...);

/* Writes Prompt to the debugger and reads a reply into Response (up to
 * MaximumResponseLength bytes). Returns the number of bytes received. */
ULONG STDCALL DbgPrompt(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength);

/* Notifies the debugger that a previously loaded image's symbols are going away;
 * the inverse of DbgLoadImageSymbols. */
VOID STDCALL DbgUnLoadImageSymbols(
    PSTRING FileName,
    PVOID ImageBase,
    ULONG_PTR ProcessId);

#endif
