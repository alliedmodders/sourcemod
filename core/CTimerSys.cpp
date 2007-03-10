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

#include "CTimerSys.h"

CTimerSystem g_Timers;

void ITimer::Initialize(ITimedEvent *pCallbacks, float fInterval, float fToExec, void *pData, int flags)
{
	m_Listener = pCallbacks;
	m_Interval = fInterval;
	m_ToExec = fToExec;
	m_pData = pData;
	m_Flags = flags;
	m_InExec = false;
	m_KillMe = false;
}

void CTimerSystem::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

void CTimerSystem::RunFrame()
{
	ITimer *pTimer;
	TimerIter iter;

	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); )
	{
		pTimer = (*iter);
		if (gpGlobals->curtime >= pTimer->m_ToExec)
		{
			pTimer->m_InExec = true;
			pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);
			if (pTimer->m_KillMe)
			{
				pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
			}
			iter = m_SingleTimers.erase(iter);
			m_FreeTimers.push(pTimer);
		} else {
			break;
		}
	}

	ResultType res;
	for (iter=m_LoopTimers.begin(); iter!=m_LoopTimers.end(); )
	{
		pTimer = (*iter);
		if (gpGlobals->curtime >= pTimer->m_ToExec)
		{
			pTimer->m_InExec = true;
			res = pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);
			if (pTimer->m_KillMe || (res == Pl_Stop))
			{
				pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
				iter = m_LoopTimers.erase(iter);
				m_FreeTimers.push(pTimer);
				continue;
			}
			pTimer->m_InExec = false;
			pTimer->m_ToExec = gpGlobals->curtime + pTimer->m_Interval;
		}
		iter++;
	}

	m_LastExecTime = gpGlobals->curtime;
}

ITimer *CTimerSystem::CreateTimer(ITimedEvent *pCallbacks, float fInterval, void *pData, int flags)
{
	ITimer *pTimer;
	TimerIter iter;
	float to_exec = gpGlobals->curtime + fInterval;

	if (m_FreeTimers.empty())
	{
		pTimer = new ITimer;
	} else {
		pTimer = m_FreeTimers.front();
		m_FreeTimers.pop();
	}

	pTimer->Initialize(pCallbacks, fInterval, to_exec, pData, flags);

	if (flags & TIMER_FLAG_REPEAT)
	{
		m_LoopTimers.push_back(pTimer);
		goto return_timer;
	}

	if (m_SingleTimers.size() >= 1)
	{
		iter = --m_SingleTimers.end();
		if ((*iter)->m_ToExec <= to_exec)
		{
			goto insert_end;
		}
	}

	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); iter++)
	{
		if ((*iter)->m_ToExec >= to_exec)
		{
			m_SingleTimers.insert(iter, pTimer);
			goto return_timer;
		}
	}

insert_end:
	m_SingleTimers.push_back(pTimer);

return_timer:
	return pTimer;
}

void CTimerSystem::FireTimerOnce(ITimer *pTimer, bool delayExec)
{
	ResultType res;

	if (pTimer->m_InExec)
	{
		return;
	}

	pTimer->m_InExec = true;
	res = pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);

	if (!(pTimer->m_Flags & TIMER_FLAG_REPEAT))
	{
		pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
		m_SingleTimers.remove(pTimer);
		m_FreeTimers.push(pTimer);
	} else {
		if (delayExec && (res != Pl_Stop) && !pTimer->m_KillMe)
		{
			pTimer->m_ToExec = gpGlobals->curtime + pTimer->m_Interval;
			pTimer->m_InExec = false;
			return;
		}
		pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
		m_LoopTimers.remove(pTimer);
		m_FreeTimers.push(pTimer);
	}
}

void CTimerSystem::KillTimer(ITimer *pTimer)
{
	TimerList *pList;

	if (pTimer->m_KillMe)
	{
		return;
	}

	if (pTimer->m_InExec)
	{
		pTimer->m_KillMe = true;
		return;
	}

	pTimer->m_InExec = true; /* The timer it's not really executed but this check needs to be done */
	pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);

	pList = (pTimer->m_Flags & TIMER_FLAG_REPEAT) ? &m_LoopTimers : &m_SingleTimers;

	pList->remove(pTimer);
	m_FreeTimers.push(pTimer);
}

void CTimerSystem::MapChange()
{
	ITimer *pTimer;
	TimerIter iter;

	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); iter++)
	{
		pTimer = (*iter);
		pTimer->m_ToExec = pTimer->m_ToExec - m_LastExecTime + gpGlobals->curtime;
	}

	for (iter=m_LoopTimers.begin(); iter!=m_LoopTimers.end(); iter++)
	{
		pTimer = (*iter);
		pTimer->m_ToExec = pTimer->m_ToExec - m_LastExecTime + gpGlobals->curtime;
	}
}
