/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Declaration of CAutDirectMusicPerformance.
// IDispatch interface for IDirectMusicPerformance.
// Only usable via aggregation within an IDirectMusicPerformance object.

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicPerformance;
typedef CAutBaseImp<CAutDirectMusicPerformance, IDirectMusicPerformance, &IID_IDirectMusicPerformance> BaseImpPerf;

class CAutDirectMusicPerformance
  : public BaseImpPerf
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicPerformance(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT SetMasterTempo(AutDispatchDecodedParams *paddp);
	HRESULT GetMasterTempo(AutDispatchDecodedParams *paddp);
	HRESULT SetMasterVolume(AutDispatchDecodedParams *paddp);
	HRESULT GetMasterVolume(AutDispatchDecodedParams *paddp);
	HRESULT SetMasterGrooveLevel(AutDispatchDecodedParams *paddp);
	HRESULT GetMasterGrooveLevel(AutDispatchDecodedParams *paddp);
	HRESULT SetMasterTranspose(AutDispatchDecodedParams *paddp);
	HRESULT GetMasterTranspose(AutDispatchDecodedParams *paddp);
	HRESULT _Trace(AutDispatchDecodedParams *paddp);
	HRESULT Rand(AutDispatchDecodedParams *paddp);

	// Helpers
	HRESULT GetMasterParam(const GUID &guid, void *pParam, DWORD dwSize); // Calls GetGlobalParam, but returns S_OK if the param hasn't been set previously.

	// data
	SmartRef::ComPtr<IDirectMusicGraph> m_scomGraph;
	short m_nTranspose;
	short m_nVolume;
	long m_lRand;

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicPerformance> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
