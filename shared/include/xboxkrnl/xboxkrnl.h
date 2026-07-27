#ifndef _XBOXKRNL_H_
#define _XBOXKRNL_H_

/*
 * MSVC IntelliSense compatibility.
 *
 * The RXDK headers are compiled by clang/Zig and use GNU/clang extensions
 * (__attribute__, __restrict__, __asm__, __typeof__, ...). Visual Studio's
 * IntelliSense parses with the MSVC front-end, which does not understand those
 * tokens, so it would report spurious errors on every title even though the real
 * build is clean. Neutralize the extensions for the MSVC front-end ONLY.
 *
 * This never affects a real build: our clang build defines __clang__ (and sets
 * _MSC_VER via -fms-compatibility), so the !defined(__clang__) guard keeps the
 * true attributes for clang while only the IDE's MSVC parser takes this branch.
 */
#if defined(_MSC_VER) && !defined(__clang__)
  #define __attribute__(x)
  #define __restrict__ __restrict
  #define __extension__
  #define __volatile__ volatile
  #define __asm__(x)
  typedef char *__gnuc_va_list;   /* picolibc's stdio.h expects this GCC type from the compiler */
  #if defined(__cplusplus)
    #define __typeof__(x) decltype(x)
    #ifndef NULL
      #define NULL 0   /* headers below use ((PVOID)0); void* -> T* is a C, not C++, conversion */
    #endif
  #else
    #define __typeof__(x) int
  #endif
#endif

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/ntstatus.h>

#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

#pragma ms_struct on

#if defined(__cplusplus)
extern "C" {
#endif

#include <xboxkrnl/types.h>
#include <xboxkrnl/api/av.h>
#include <xboxkrnl/api/dbg.h>
#include <xboxkrnl/api/ex.h>
#include <xboxkrnl/api/hal.h>
#include <xboxkrnl/api/io.h>
#include <xboxkrnl/api/ke.h>
#include <xboxkrnl/api/mm.h>
#include <xboxkrnl/api/nt.h>
#include <xboxkrnl/api/ob.h>
#include <xboxkrnl/api/ps.h>
#include <xboxkrnl/api/rtl.h>
#include <xboxkrnl/api/xc.h>
#include <xboxkrnl/api/xbox.h>
#include <xboxkrnl/api/misc.h>

#if defined(__cplusplus)
}
#endif

#pragma ms_struct off
#pragma clang diagnostic pop

#endif
