#ifndef RXDK_INCLUDE_NTRTL_H
#define RXDK_INCLUDE_NTRTL_H

#if (defined(_XAPIP_) || defined(_BASEP_) || defined(_DLLP_)) && !defined(_NTRTLP_)
#include <xapi/ntrtl.h>
#else
#include <sdk/ntrtl.h>
#endif

#endif
