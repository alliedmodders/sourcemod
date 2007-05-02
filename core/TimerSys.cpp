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

#include "TimerSys.h"

TimerSystem g_Timers;
TickInfo g_SimTicks;

inline float GetSimulatedTime()
{
	if (g_SimTicks.ticking)
	{
		return gpGlobals->curtime;
	} else {
		return g_SimTicks.ticktime;
	}
}

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

TimerSystem::~TimerSystem()
{
	CStack<ITimer *>::iterator iter;
	for (iter=m_FreeTimers.begin(); iter!=m_FreeTimers.end(); iter++)
	{
		delete (*iter);
	}
	m_FreeTimers.popall();
}

void TimerSystem::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

void TimerSystem::OnSourceModLevelChange(const char *mapName)
{
	MapChange();
}

void TimerSystem::RunFrame()
{
	ITimer *pTimer;
	TimerIter iter;

	float curtime = GetSimulatedTime();
	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); )
	{
		pTimer = (*iter);
		if (curtime >= pTimer->m_ToExec)
		{
			pTimer->m_InExec = true;
			pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);
			pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
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
		if (curtime >= pTimer->m_ToExec)
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
			pTimer->m_ToExec = curtime + pTimer->m_Interval;
		}
		iter++;
	}

	m_LastExecTime = curtime;
}

ITimer *TimerSystem::CreateTimer(ITimedEvent *pCallbacks, float fInterval, void *pData, int flags)
{
	ITimer *pTimer;
	TimerIter iter;
	float to_exec = GetSimulatedTime() + fInterval;

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

void TimerSystem::FireTimerOnce(ITimer *pTimer, bool delayExec)
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
		if ((res != Pl_Stop) && !pTimer->m_KillMe)
		{
			if (delayExec)
			{
				pTimer->m_ToExec = GetSimulatedTime() + pTimer->m_Interval;
			}
			pTimer->m_InExec = false;
			return;
		}
		pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
		m_LoopTimers.remove(pTimer);
		m_FreeTimers.push(pTimer);
	}
}

void TimerSystem::KillTimer(ITimer *pTimer)
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

void TimerSystem::MapChange()
{
	ITimer *pTimer;
	TimerIter iter;

	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); iter++)
	{
		pTimer = (*iter);
		pTimer->m_ToExec = pTimer->m_ToExec - m_LastExecTime + GetSimulatedTime();
	}

	for (iter=m_LoopTimers.begin(); iter!=m_LoopTimers.end(); iter++)
	{
		pTimer = (*iter);
		pTimer->m_ToExec = pTimer->m_ToExec - m_LastExecTime + GetSimulatedTime();
	}

	m_LastExecTime = GetSimulatedTime();
}
