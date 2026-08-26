/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* This is a class to manage tracking mutes for the SeqTrack and BandTrack. */

#ifndef _PCHMAP__
#define _PCHMAP__
#include "dmusicip.h"
#include "..\dmstyle\tlist.h"

struct PCHMAP_ITEM
{
	MUSIC_TIME	mtNext;
	DWORD		dwPChannel;
	DWORD		dwPChMap;
	BOOL		fMute;
};

class CPChMap
{
public:
	CPChMap();
	~CPChMap();
	void Reset(void);
	void GetInfo( DWORD dwPCh, MUSIC_TIME mtTime, MUSIC_TIME mtOffset, DWORD dwGroupBits,
				  IDirectMusicPerformance* pPerf, BOOL* pfMute, DWORD* pdwNewPCh , BOOL fClockTime);
private:
	TList<PCHMAP_ITEM>	m_PChMapList;
};
#endif // _PCHMAP__

