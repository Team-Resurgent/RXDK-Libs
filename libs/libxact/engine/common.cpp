
#include "stdafx.h"

#include "common.h"

//
// RXDK 5849 uplift: the leak added a GLOBAL replacement operator new/delete (routed
// through XactMemAlloc/XactMemFree). The 5849 retail xacteng.lib exports no such global
// operators -- it uses the CRT's, like every other RXDK lib (libdsound/libd3d8/...).
// Providing our own here produced a duplicate-symbol link error against the CRT's, so
// XACT now uses the CRT operator new for its C++ objects; XactMemAlloc/XactMemFree remain
// for XACT's own internal buffer allocations (pooled via ExAllocatePoolWithTag).
//
