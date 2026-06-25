#pragma once
#define RXDK_UUID_BRIDGE_H

/* uuid slice builds COM/RPC IDL against the SDK (was -DXAPI_UUID_BUILD). */
#ifndef XAPI_UUID_BUILD
#define XAPI_UUID_BUILD 1
#endif



/*
 * Force-included for libxapi uuid slice (COM/RPC IDL objects).
 * sdk/winbase.h omits Interlocked* decls when _NTOS_ is set (k32 provides them).
 */

#ifndef _NTOS_
#define _NTOS_
#endif

