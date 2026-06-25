#ifndef RXDK_INCLUDE_NTRTL_H
#define RXDK_INCLUDE_NTRTL_H

#if (defined(_XAPIP_) || defined(_BASEP_) || defined(_DLLP_)) && !defined(_NTRTLP_)
#include "rxdk_ntrtl.h"
#else
#include <ntrtl_sdk.h>
#endif

#endif
