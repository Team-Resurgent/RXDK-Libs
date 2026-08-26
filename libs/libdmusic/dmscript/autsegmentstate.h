/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Declaration of CAutDirectMusicSegmentState.
// IDispatch interface for IDirectMusicSegmentState.
// Only usable via aggregation within an IDirectMusicSegmentState object.

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicSegmentState;
typedef CAutBaseImp<CAutDirectMusicSegmentState, IDirectMusicSegmentState, &IID_IDirectMusicSegmentState> BaseImpSegSt;

class CAutDirectMusicSegmentState
  : public BaseImpSegSt
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicSegmentState(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT IsPlaying(AutDispatchDecodedParams *paddp);
	HRESULT Stop(AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicSegmentState> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
