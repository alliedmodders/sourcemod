/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include <time.h>
#include "TimerSys.h"
#include "ForwardSys.h"
#include "sourcemm_api.h"
#include "frame_hooks.h"

TimerSystem g_Timers;
float g_fUniversalTime = 0.0f;
const float *g_pUniversalTime = &g_fUniversalTime;

ConVar sm_time_adjustment("sm_time_adjustment", "0", 0, "Adjusts the server time in seconds");

inline float GetSimulatedTime()
{
	return g_fUniversalTime;
}

time_t GetAdjustedTime(time_t *buf)
{
	time_t val = time(NULL) + sm_time_adjustment.GetInt();
	if (buf)
	{
		*buf = val;
	}
	return val;
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

TimerSystem::TimerSystem()
{
	m_fnTimeLeft = NULL;
	m_bHasMapTickedYet = false;
	m_fLastTickedTime = 0.0f;
	m_LastExecTime = 0.0f;
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
	m_pOnGameFrame = g_Forwards.CreateForward("OnGameFrame", ET_Ignore, 0, NULL);
}

void TimerSystem::OnSourceModShutdown()
{
	g_Forwards.ReleaseForward(m_pOnGameFrame);
}

void TimerSystem::OnSourceModLevelChange(const char *mapName)
{
	MapChange(true);
}

void TimerSystem::OnSourceModLevelEnd()
{
	m_bHasMapTickedYet = false;
}

void TimerSystem::GameFrame(bool simulating)
{
	if (simulating && m_bHasMapTickedYet)
	{
		g_fUniversalTime += gpGlobals->curtime - m_fLastTickedTime;
	}
	else 
	{
		g_fUniversalTime += gpGlobals->interval_per_tick;
	}

	m_fLastTickedTime = gpGlobals->curtime;
	m_bHasMapTickedYet = true;

	if (g_fUniversalTime - m_LastExecTime >= 0.1)
	{
		RunFrame();
	}

	RunFrameHooks();

	if (m_pOnGameFrame->GetFunctionCount())
	{
		m_pOnGameFrame->Execute(NULL);
	}
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

	if (m_fnTimeLeft != NULL && m_MapEndTimers.size())
	{
		float time_left = m_fnTimeLeft();
		for (iter=m_MapEndTimers.begin(); iter!=m_MapEndTimers.end();)
		{
			pTimer = (*iter);
			if ((*iter)->m_Interval < time_left)
			{
				pTimer->m_InExec = true;
				pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);
				pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
				iter = m_MapEndTimers.erase(iter);
				m_FreeTimers.push(pTimer);
			} else {
				break;
			}
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

	if ((flags & TIMER_FLAG_BEFORE_MAP_END)
		&& m_fnTimeLeft == NULL)
	{
		return NULL;
	}

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

	if (flags & TIMER_FLAG_BEFORE_MAP_END)
	{
		for (iter=m_MapEndTimers.begin(); iter!=m_MapEndTimers.end(); iter++)
		{
			if (fInterval >= (*iter)->m_Interval)
			{
				m_MapEndTimers.insert(iter, pTimer);
				goto return_timer;
			}
		}

		m_MapEndTimers.push_back(pTimer);
	} else {
		if (m_SingleTimers.size() >= 1)
		{
			iter = --m_SingleTimers.end();
			if ((*iter)->m_ToExec <= to_exec)
			{
				goto normal_insert_end;
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

normal_insert_end:
		m_SingleTimers.push_back(pTimer);
	}

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
		if (pTimer->m_Flags & TIMER_FLAG_BEFORE_MAP_END)
		{
			m_MapEndTimers.remove(pTimer);
		} else {
			m_SingleTimers.remove(pTimer);
		}
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

	if (pTimer->m_Flags & TIMER_FLAG_REPEAT)
	{
		m_LoopTimers.remove(pTimer);
	} else if (pTimer->m_Flags & TIMER_FLAG_BEFORE_MAP_END) {
		m_MapEndTimers.remove(pTimer);
	} else {
		m_SingleTimers.remove(pTimer);
	}

	m_FreeTimers.push(pTimer);
}

CStack<ITimer *> s_tokill;
void TimerSystem::MapChange(bool real_mapchange)
{
	ITimer *pTimer;
	TimerIter iter;

	for (iter=m_SingleTimers.begin(); iter!=m_SingleTimers.end(); iter++)
	{
		pTimer = (*iter);
		if (real_mapchange && (pTimer->m_Flags & TIMER_FLAG_NO_MAPCHANGE))
		{
			s_tokill.push(pTimer);
		}
	}

	for (iter=m_LoopTimers.begin(); iter!=m_LoopTimers.end(); iter++)
	{
		pTimer = (*iter);
		if (real_mapchange && (pTimer->m_Flags & TIMER_FLAG_NO_MAPCHANGE))
		{
			s_tokill.push(pTimer);
		}
	}

	if (real_mapchange)
	{
		for (iter=m_MapEndTimers.begin(); iter!=m_MapEndTimers.end(); iter++)
		{
			pTimer = (*iter);
			s_tokill.push(pTimer);
		}
	}

	while (!s_tokill.empty())
	{
		KillTimer(s_tokill.front());
		s_tokill.pop();
	}
}

SM_TIMELEFT_FUNCTION TimerSystem::SetTimeLeftFunction(SM_TIMELEFT_FUNCTION fn)
{
	SM_TIMELEFT_FUNCTION old = m_fnTimeLeft;
	m_fnTimeLeft = fn;
	return old;
}

