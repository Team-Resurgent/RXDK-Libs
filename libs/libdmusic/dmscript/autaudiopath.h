/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Declaration of CAutDirectMusicAudioPath.
// IDispatch interface for IDirectMusicAudioPath.
// Only usable via aggregation within an IDirectMusicAudioPath object.

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicAudioPath;
typedef CAutBaseImp<CAutDirectMusicAudioPath, IDirectMusicAudioPath, &IID_IDirectMusicAudioPath> BaseImpAudioPath;

class CAutDirectMusicAudioPath
  : public BaseImpAudioPath
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicAudioPath(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT SetVolume(AutDispatchDecodedParams *paddp);
	HRESULT GetVolume(AutDispatchDecodedParams *paddp);

	LONG m_lVolume;

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicAudioPath> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
