/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "sm_queue.h"
#include <IDBDriver.h>

template <class T>
class PrioQueue
{
private:
	Queue<T> m_HighQueue;
	Queue<T> m_NormalQueue;
	Queue<T> m_LowQueue;
public:
	inline Queue<T> & GetQueue(PrioQueueLevel level)
	{
		if (level == PrioQueue_High)
		{
			return m_HighQueue;
		} else if (level == PrioQueue_Normal) {
			return m_NormalQueue;
		} else {
			return m_LowQueue;
		}
	}
	inline Queue<T> & GetLikelyQueue()
	{
		if (!m_HighQueue.empty())
		{
			return m_HighQueue;
		}
		if (!m_NormalQueue.empty())
		{
			return m_NormalQueue;
		}
		return m_LowQueue;
	}
};
