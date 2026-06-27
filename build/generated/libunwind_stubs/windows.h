/* RXDK stub: libunwind includes <windows.h> under _WIN32, but we use the
   DWARF (not SEH) unwinder, so none of the Win32 APIs are referenced. This
   empty shim shadows mingw windows.h (which pulls stralign.h -> wcscpy). */
#ifndef _RXDK_LIBUNWIND_WINDOWS_H
#define _RXDK_LIBUNWIND_WINDOWS_H
#endif
