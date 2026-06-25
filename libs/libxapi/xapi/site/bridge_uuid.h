#ifndef RXDK_UUID_BRIDGE_H
#define RXDK_UUID_BRIDGE_H

/*
 * Force-included for libxapi uuid slice (COM/RPC IDL objects).
 * sdk/winbase.h omits Interlocked* decls when _NTOS_ is set (k32 provides them).
 */

#ifndef _NTOS_
#define _NTOS_
#endif

#endif /* RXDK_UUID_BRIDGE_H */
