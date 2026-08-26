/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Declaration of CAutDirectMusicAudioPathConfig.
// IDispatch interface for IUnknown.
// Only usable via aggregation within an IUnknown object.

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicAudioPathConfig;
typedef CAutBaseImp<CAutDirectMusicAudioPathConfig, IDirectMusicObject, &IID_IPersistStream> BaseImpAPConfig;

class CAutDirectMusicAudioPathConfig
  : public BaseImpAPConfig
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicAudioPathConfig(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT Load(AutDispatchDecodedParams *paddp);
	HRESULT Create(AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicAudioPathConfig> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
