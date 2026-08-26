/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of CNotificationItem and the notification-tracking list used by
 * the DirectMusic performance.
 */

#include "alist.h"
#include "dmusicip.h"
#include "debug.h"

#ifndef __NTFYLIST_H_
#define __NTFYLIST_H_

class CNotificationItem : public AListItem
{
public:
	CNotificationItem* GetNext()
	{
		return (CNotificationItem*)AListItem::GetNext();
	};
public:
	GUID	guidNotificationType;
    BOOL    fFromPerformance;
};

class CNotificationList : public AList
{
public:
    CNotificationItem* GetHead() 
	{
		return (CNotificationItem*)AList::GetHead();
	};
    CNotificationItem* RemoveHead() 
	{
		return (CNotificationItem*)AList::RemoveHead();
	};
    CNotificationItem* GetItem(LONG lIndex) 
	{
		return (CNotificationItem*) AList::GetItem(lIndex);
	};
	void Clear(void)
	{
		CNotificationItem* pTrack;
		while( pTrack = RemoveHead() )
		{
			delete pTrack;
		}
	}
};

#endif // __NTFYLIST_H_
