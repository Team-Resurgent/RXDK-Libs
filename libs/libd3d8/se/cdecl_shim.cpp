/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

//------------------------------------------------------------------------------
// cdecl_shim.cpp -- compiled with -fdefault-calling-conv=cdecl (overriding the
// libd3d8-wide -fdefault-calling-conv=stdcall; see build.zig).
//
// libd3d8 is built /Gz (stdcall default) like the Xbox D3D8.  Under a stdcall
// default Clang IGNORES an explicit __cdecl on a function-pointer TYPE at an
// INDIRECT call site -- it emits a stdcall `sub esp,N` re-reserve after the call,
// but the __cdecl callee never pops, so the caller leaks N bytes of stack.  In the
// NV2A interrupt dispatch (mpintr.cpp) that leak makes SoftwareMethod's esp-
// relative epilogue return from the wrong slot -> execute-from-stack crash (the
// Notifier "hang").  Clang honors __cdecl on function DEFINITIONS and on direct
// named calls (cf. site/cdecl_libc.h), just not on this indirect call; an explicit
// cast at the call site does not help.
//
// The only reliable fix is to make the indirect call happen where the default IS
// cdecl.  This one tiny __stdcall trampoline is that place: the interrupt code
// calls it with the ordinary stdcall convention, and here -- default cdecl -- the
// pfn(arg) indirect call is emitted with correct caller-cleanup.  Every D3D
// callback (D3DCALLBACK/D3DVBLANKCALLBACK/D3DSWAPCALLBACK) is a __cdecl function of
// one 4-byte argument (a DWORD context or a data pointer), so one shim covers all.
//------------------------------------------------------------------------------

extern "C" __attribute__((stdcall))
void RxdkInvokeCdeclCallback(void (__attribute__((cdecl)) * pfn)(void*), void* arg)
{
    pfn(arg);
}
