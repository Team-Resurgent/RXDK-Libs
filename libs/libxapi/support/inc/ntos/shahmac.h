/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Routines to calculate an SHA HMAC given a material key and a series of
 * piecemeal data blobs.
 */

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

