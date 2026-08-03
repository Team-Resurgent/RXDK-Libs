
#ifndef __COMMON_H__
#define __COMMON_H__

//
// RXDK title-side adaptation of the leak XACT runtime/common/common.h.
//
// The original pulled the NT kernel header set (nt.h/ntrtl/nturtl/ntos.h/pci.h)
// and the PRIVATE dsoundp.h. libxact is built TITLE-SIDE: site/bridge_xact.h
// (force-included before every TU) already supplies the title umbrella --
// <xtl.h> (Win32 + xboxkrnl: LIST_ENTRY, CRITICAL_SECTION, Ke* timer/DPC/IRQL,
// Ex* pool) and the PUBLIC <dsound.h> (IDirectSoundBuffer/Stream, DSMIXBINS,
// DS3D*/DSI3DL2*/DSFILTER/DSENVELOPE/DSLFO/DSEFFECTIMAGE* and REFERENCE_TIME).
// So this header only layers the XACT-private helper machinery on top.
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
// C runtime the helpers lean on (already provided title-side via picolibc, but
// keep the faithful includes so the machinery is self-describing).
//

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

//
// Private includes (the helper machinery -- copied verbatim from the leak
// runtime/common tree). refcount/debug/drvhlp/ntlist live in namespace XACT
// exactly as the original did.
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
// The leak defined these as `static` free functions in the header (one copy per
// TU). Clang rejects a `static`/`inline` GLOBAL replacement operator new/delete,
// so the single definition lives in engine/common.cpp (the "common" TU) and the
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
