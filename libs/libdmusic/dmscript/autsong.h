/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

// Declaration of CAutDirectMusicSong.
// IDispatch interface for IDirectMusicSong.
// Only usable via aggregation within an IDirectMusicSong object.

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicSong;
typedef CAutBaseImp<CAutDirectMusicSong, IDirectMusicSong, &IID_IDirectMusicSong> BaseImpSong;

class CAutDirectMusicSong
  : public BaseImpSong
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicSong(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT Load(AutDispatchDecodedParams *paddp);
	HRESULT Recompose(AutDispatchDecodedParams *paddp);
	HRESULT Play(AutDispatchDecodedParams *paddp);
	HRESULT GetSegment(AutDispatchDecodedParams *paddp);
	HRESULT Stop(AutDispatchDecodedParams *paddp);
	HRESULT DownloadSoundData(AutDispatchDecodedParams *paddp);
	HRESULT UnloadSoundData(AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicSong> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
