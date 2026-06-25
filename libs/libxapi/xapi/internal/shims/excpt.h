#ifndef RXDK_INCLUDE_EXCPT_H
#define RXDK_INCLUDE_EXCPT_H

/*
 * sdk/nt.h pulls <excpt.h> for SEH types. Redirect away from xdk/excpt.h
 * (CRT stub) to the sdk CRT header ntdef/ntrtl expect.
 */

#include <sdk/crt/excpt.h>

#endif
