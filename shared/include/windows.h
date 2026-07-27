#pragma once
/*
 * RXDK compatibility umbrella.
 *
 * The Xbox XDK's canonical umbrella header is <xtl.h>, but a lot of legacy / ported code
 * (and the SDK's own winsockx.h) does `#include <windows.h>`. Without this shim that
 * include falls through to the host toolchain's <windows.h> (zig's MinGW headers), which
 * pulls in wingdi/winuser and fails hard on the freestanding Xbox target.
 *
 * Map <windows.h> onto <xtl.h> and define the classic Platform SDK include guard
 * (_INC_WINDOWS) so headers guarded on it (winsockx.h) see it as already satisfied.
 */
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#include <xtl.h>
#endif /* _INC_WINDOWS */
