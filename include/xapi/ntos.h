#ifndef RXDK_XAPI_NTOS_H
#define RXDK_XAPI_NTOS_H

/*
 * Shadow of leak ntos.h for libxapi USB/driver slices.
 *
 * Vendor init.h / ntosdef.h use leak sdk/ntdef layouts — not public xboxkrnl
 * types.h (KAPC, KDPC, etc. differ). nt.h redirect supplies sdk/ntdef.h.
 */

#ifndef _NTOS_
#define _NTOS_

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpragma-pack"

#include "xapi_ntos_vendor.h"

#pragma clang diagnostic pop

#ifdef __cplusplus
}
#endif

#endif /* _NTOS_ */
#endif /* RXDK_XAPI_NTOS_H */
