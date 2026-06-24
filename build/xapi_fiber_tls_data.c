/*
 * RXDK: image .tls template for fiber state (separate TU for Clang emutls).
 */

#include "basedll.h"
#pragma hdrstop

__declspec(thread) PVOID RxdkXapiCurrentFiberTemplate;
__declspec(thread) XFIBER RxdkXapiThreadFiberDataTemplate;
