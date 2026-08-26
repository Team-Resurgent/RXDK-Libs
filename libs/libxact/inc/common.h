/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

//
// Title-side common header for the XACT runtime. It layers the XACT-private
// helper machinery on top of the title umbrella.
//
// libxact is built TITLE-SIDE: site/bridge_xact.h (force-included before every
// TU) already supplies the title umbrella -- <xtl.h> (Win32 + xboxkrnl:
// LIST_ENTRY, CRITICAL_SECTION, Ke* timer/DPC/IRQL, Ex* pool) and the PUBLIC
// <dsound.h> (IDirectSoundBuffer/Stream, DSMIXBINS, DS3D*/DSI3DL2*/DSFILTER/
// DSENVELOPE/DSLFO/DSEFFECTIMAGE* and REFERENCE_TIME). So this header only
// layers the XACT-private helper machinery on top.
//

//
// Preprocessor definitions
//

#if DBG && !defined(DEBUG)
#define DEBUG
#endif

#if defined(DEBUG) && !defined(VALIDATE_PARAMETERS)
#define VALIDATE_PARAMETERS
#endif

//
// C runtime the helpers lean on (provided title-side via picolibc; the includes
// are kept so the machinery is self-contained).
//

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

//
// Private includes (the helper machinery). macros/debug/drvhlp/ntlist/refcount
// live in namespace XACT.
//

namespace XACT {

#include "macros.h"
#include "debug.h"
#include "drvhlp.h"
#include "ntlist.h"
#include "refcount.h"

}//namespace

#include "memmgr.h"

//
// New and delete overrides
//
// Clang rejects a `static`/`inline` GLOBAL replacement operator new/delete, so
// the single definition lives in engine/common.cpp (the "common" TU) and the
// NEW/DELETE macros below just use the implicitly-declared global operators.
//

#ifdef __cplusplus


#ifdef TRACK_MEMORY_USAGE

#define NEW(type) \
    new(__FILE__, __LINE__, #type) type

#define NEW_A(type, count) \
    new(__FILE__, __LINE__, #type) type [count]

#else // TRACK_MEMORY_USAGE

#define NEW(type) \
    new type

#define NEW_A(type, count) \
    new type [count]

#endif // TRACK_MEMORY_USAGE

#undef DELETE
#define DELETE(p) \
    { \
        if(p) \
        { \
            delete (p); \
            (p) = NULL; \
        } \
    }

#define DELETE_A(p) \
    { \
        if(p) \
        { \
            delete [] (p); \
            (p) = NULL; \
        } \
    }


#endif // __cplusplus


#endif // __COMMON_H__
