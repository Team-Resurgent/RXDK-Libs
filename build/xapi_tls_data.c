/*
 * RXDK: image .tls template for Win32 TLS slots (separate TU for Clang emutls).
 */

#include "basedll.h"
#pragma hdrstop

__declspec(thread) PVOID XapiTlsSlots[TLS_MINIMUM_AVAILABLE];
