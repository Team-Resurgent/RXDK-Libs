/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Debug helpers for the XGRAPHICS namespace: the DBG_CHECK / NULL_CHECK
 * parameter-validation macros, the DPF debug-print family, and the assertion
 * macros (ASSERT, ASSERTMSG, DXGASSERT, DXGRIP, WARNING, UNIMPLEMENTED). On
 * free/retail builds these all collapse to no-ops or constant FALSE so the
 * associated code is compiled out.
 */

#pragma once

namespace XGRAPHICS
{

#include <xdbg.h>

#if DBG

    // DBG_CHECK is to be used to for any parameter validation checks, 
    // in the form:
    //
    //      if (DBG_CHECK(dwFlags & INVALID_FLAGS))
    //          return DDERR_UNSUPPORTED;
    //
    // On free, retail builds, the macro gets converted to a constant '0'
    // and the compiler will remove all the associated code.
       
    #define DBG_CHECK(exp) (exp)
    
    // NULL_CHECK is to be used for any parameter NULL pointer checks,
    // in the form:
    //
    //      if (NULL_CHECK(pFoo))
    //          return DDERR_INVALIDPARAMETER;
    
    #define NULL_CHECK(p) ((p) == NULL)
    
#else

    #define DBG_CHECK(exp) FALSE
    
    #define NULL_CHECK(p) FALSE
    
#endif

#if DBG

    void DPF3(const char* message, va_list list);
    void DPF2(const char* message,...);
    void DPF(int level,const char* message,...);

    VOID DXGRIP(PCHAR Format, ...);
    VOID WARNING(PCHAR Format, ...);

    #undef ASSERT
    #define ASSERT(cond)   \
        {                  \
            if (! (cond))  \
            {              \
                DXGRIP("Assertion failure: %s", #cond); \
            }              \
        }

    #undef ASSERTMSG
    VOID ASSERTMSG(BOOL cond, PCHAR Format, ...);

    #define DPF_ERR(msg) DXGRIP(msg)
    #define D3D_ERR(msg) DXGRIP(msg)
    #define DXGASSERT(cond) ASSERT(cond)
    #define DDASSERT(cond) ASSERT(cond)
    #define UNIMPLEMENTED() DXGRIP("Function not yet implemented")

#else

    #define DPF3
    #define DPF2
    #define DPF
    
    #define DXGRIP
    #define WARNING

    #undef ASSERT
    #define ASSERT(cond) {}
    #undef ASSERTMSG
    #define ASSERTMSG

    #define DPF_ERR
    #define D3D_ERR
    #define DXGASSERT
    #define DDASSERT
    #define UNIMPLEMENTED

#endif

}
