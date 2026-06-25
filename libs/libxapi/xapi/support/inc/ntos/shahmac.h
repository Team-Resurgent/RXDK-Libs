/*++

Copyright (c) Microsoft Corporation

Description:
	Definition of routines to calculate an SHA HMAC
	given a material key and a series of piecemeal 
	data blobs

Module Name:

	shahmac.h

--*/

#pragma once
#define __SHAHMAC_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <xcrypt.h>

typedef BYTE XSHAHMAC_CONTEXT[XC_SERVICE_SHA_CONTEXT_SIZE];

XBOXAPI
VOID 
__attribute__((__stdcall__)) 
XShaHmacInitialize(
	IN PBYTE				pbKey,
	IN DWORD				cbKey,
	IN OUT XSHAHMAC_CONTEXT	Shactx
	);

XBOXAPI
VOID
__attribute__((__stdcall__)) 
XShaHmacUpdate(
	IN XSHAHMAC_CONTEXT	Shactx,
	IN PBYTE			pbData,
	IN DWORD			cbData
	);

XBOXAPI
VOID 
__attribute__((__stdcall__)) 
XShaHmacComputeFinal(
	IN XSHAHMAC_CONTEXT	Shactx,
	IN PBYTE			pbKey,
	IN DWORD			cbKey,
	OUT PBYTE			pbHmac
	);

#ifdef __cplusplus
}
#endif

