/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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
