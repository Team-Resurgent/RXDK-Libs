/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Xbox statistics APIs -- per-title stat read/write and leaderboards.
 */

#include "xonp.h"
#include "xonver.h"

HRESULT
CXo::XOnlineStatSet(
    IN DWORD dwNumStatsSpecs,
    IN PXONLINE_STAT_SPEC pStatSpecs,
    IN HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask
)
{
    return E_NOTIMPL;
}

HRESULT 
CXo::XOnlineStatGet(
    IN DWORD dwNumStatSpecs,
    IN OUT PXONLINE_STAT_SPEC pStatSpecs,
    IN HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask
)
{
    return E_NOTIMPL;
}

HRESULT 
CXo::XOnlineStatLeaderEnumerate(
	IN XUID* pxuidPagePivot,                           
	IN DWORD dwPageStart,                                              
	IN DWORD dwPageSize,
	IN DWORD dwLeaderboardID,
	IN DWORD dwNumStatsPerUser,
	IN DWORD *pStatsPerUser,
	OUT PXONLINE_STAT_USER pUsers,
	OUT PXONLINE_STAT pStats,
	IN HANDLE hWorkEvent,
	OUT PXONLINETASK_HANDLE phTask
)
{
    return E_NOTIMPL;
}

HRESULT
CXo::XOnlineStatLeaderEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD *pdwReturnedResults
)
{
    return E_NOTIMPL;
}

	
