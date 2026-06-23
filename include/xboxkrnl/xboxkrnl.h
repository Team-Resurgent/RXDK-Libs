#ifndef _XBOXKRNL_H_
#define _XBOXKRNL_H_

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
