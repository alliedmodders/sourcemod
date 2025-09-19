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

#include <time.h>
#include "TimerSys.h"
#include "sourcemm_api.h"
#include "frame_hooks.h"
#include "ConVarManager.h"
#include "logic_bridge.h"

#define TIMER_MIN_ACCURACY 0.1

TimerSystem g_Timers;
double g_fUniversalTime = 0.0f;
float g_fGameStartTime = 0.0f;       /* Game game start time, non-universal */
double g_fTimerThink = 0.0f;         /* Timer's next think time */
const double *g_pUniversalTime = &g_fUniversalTime;
ConVar *mp_timelimit = NULL;
int g_TimeLeftMode = 0;

ConVar sm_time_adjustment("sm_time_adjustment", "0", 0, "Adjusts the server time in seconds");

inline double GetSimulatedTime()
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

class DefaultMapTimer : 
	public IMapTimer,
	public SMGlobalClass,
	public IConVarChangeListener
{
public:

#if SOURCE_ENGINE == SE_BMS
	static constexpr int kMapTimeScaleFactor = 60;
#else
	static constexpr int kMapTimeScaleFactor = 1;
#endif
	
	DefaultMapTimer()
	{
		m_bInUse = false;
	}

	void OnSourceModLevelChange(const char *mapName)
	{
		g_fGameStartTime = 0.0f;
	}

	int GetMapTimeLimit()
	{
		return (mp_timelimit->GetInt() / kMapTimeScaleFactor);
	}

	void SetMapTimerStatus(bool enabled)
	{
		if (enabled && !m_bInUse)
		{
			Enable();
		} 
		else if (!enabled && m_bInUse)
		{
			Disable();
		}
		m_bInUse = enabled;
	}

	void ExtendMapTimeLimit(int extra_time)
	{
		if (extra_time == 0)
		{
			mp_timelimit->SetValue(0);
			return;
		}

		extra_time /= (60 / kMapTimeScaleFactor);

		mp_timelimit->SetValue(mp_timelimit->GetInt() + extra_time);
	}

	void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue)
	{
		g_Timers.MapTimeLeftChanged();
	}

private:
	void Enable()
	{
		g_ConVarManager.AddConVarChangeListener("mp_timelimit", this);
	}

	void Disable()
	{
		g_ConVarManager.RemoveConVarChangeListener("mp_timelimit", this);
	}

private:
	bool m_bInUse;
} s_DefaultMapTimer;

/**
 * If the ticking process has run amok (should be impossible), we 
 * take care of this by "skipping" the in-between time, to prevent 
 * a bazillion times from firing on accident.  This has the result  
 * that a drastic jump in time will continue acting normally.  Users 
 * may not expect this, but... I think it is the best solution.
 */
inline double CalcNextThink(double last, float interval, bool useTickInterval = false)
{
    const float intervalAccuracy = useTickInterval ? gpGlobals->interval_per_tick : TIMER_MIN_ACCURACY;
	if (g_fUniversalTime - last - interval <= intervalAccuracy)
	{
		return last + interval;
	}
	else
	{
		return g_fUniversalTime + interval;
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
    m_HighSpeed = false;
}

TimerSystem::TimerSystem()
{
	m_pMapTimer = NULL;
	m_bHasMapTickedYet = false;
	m_bHasMapSimulatedYet = false;
	m_fLastTickedTime = 0.0f;
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
	sharesys->AddInterface(NULL, this);
	m_pOnGameFrame = forwardsys->CreateForward("OnGameFrame", ET_Ignore, 0, NULL);
	m_pOnMapTimeLeftChanged = forwardsys->CreateForward("OnMapTimeLeftChanged", ET_Ignore, 0, NULL);
}

void TimerSystem::OnSourceModGameInitialized()
{
	mp_timelimit = icvar->FindVar("mp_timelimit");

	if (m_pMapTimer == NULL && mp_timelimit != NULL)
	{
		SetMapTimer(&s_DefaultMapTimer);
	}
}

void TimerSystem::OnSourceModShutdown()
{
	SetMapTimer(NULL);
	forwardsys->ReleaseForward(m_pOnGameFrame);
	forwardsys->ReleaseForward(m_pOnMapTimeLeftChanged);
}

void TimerSystem::OnSourceModLevelEnd()
{
	m_bHasMapTickedYet = false;
	m_bHasMapSimulatedYet = false;
}

void TimerSystem::GameFrame(bool simulating)
{
	if (simulating && m_bHasMapTickedYet)
	{
		g_fUniversalTime += gpGlobals->curtime - m_fLastTickedTime;
		if (!m_bHasMapSimulatedYet)
		{
			m_bHasMapSimulatedYet = true;
			MapTimeLeftChanged();
		}
	}
	else 
	{
		g_fUniversalTime += gpGlobals->interval_per_tick;
	}

	m_fLastTickedTime = gpGlobals->curtime;
	m_bHasMapTickedYet = true;

    const bool timerThink = g_fUniversalTime >= g_fTimerThink;
    RunFrame(timerThink);

	if (timerThink)
	{
		g_fTimerThink = CalcNextThink(g_fTimerThink, gpGlobals->interval_per_tick);
	}

	RunFrameHooks(simulating);

	if (m_pOnGameFrame->GetFunctionCount())
	{
		m_pOnGameFrame->Execute(NULL);
	}
}

void TimerSystem::ProcessRepeatTimers(double curtime, List<ITimer*>& timerList, bool isHighSpeed)
{
    ITimer *pTimer;
	TimerIter iter;

    ResultType res;
	for (iter=timerList.begin(); iter!=timerList.end(); )
	{
		pTimer = (*iter);
		if (curtime >= pTimer->m_ToExec)
		{
			pTimer->m_InExec = true;
			res = pTimer->m_Listener->OnTimer(pTimer, pTimer->m_pData);
			if (pTimer->m_KillMe || (res == Pl_Stop))
			{
				pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
				iter = timerList.erase(iter);
				m_FreeTimers.push(pTimer);
				continue;
			}
			pTimer->m_InExec = false;
			pTimer->m_ToExec = CalcNextThink(pTimer->m_ToExec, pTimer->m_Interval, isHighSpeed);
		}
		iter++;
	}
}

void TimerSystem::RunFrame(bool timerThink)
{
	const double curtime = GetSimulatedTime();

    //// One-off timers
    ITimer *pTimer;
	TimerIter iter;
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
		} 
		else 
		{
			break;
		}
	}

    //// Repeating timers
    // Most repeating timers do not need to be updated every frame 
    if (timerThink)
    {
        ProcessRepeatTimers(curtime, m_LowSpeedLoopTimers, false);
    }

    // High speed repeating timers will always update
    ProcessRepeatTimers(curtime, m_HighSpeedLoopTimers, true);
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
        List<ITimer*> timerList = pTimer->m_Flags & TIMER_FLAG_TICK_PRECISE ? m_HighSpeedLoopTimers : m_LowSpeedLoopTimers; 
        timerList.push_back(pTimer);
		goto return_timer;
	}

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
	} 
	else 
	{
		if ((res != Pl_Stop) && !pTimer->m_KillMe)
		{
			if (delayExec)
			{
				pTimer->m_ToExec = GetSimulatedTime() + pTimer->m_Interval;
			}
			pTimer->m_InExec = false;
			return;
		}

        List<ITimer*> timerList = pTimer->m_Flags & TIMER_FLAG_TICK_PRECISE ? m_HighSpeedLoopTimers : m_LowSpeedLoopTimers; 
		pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);
		timerList.remove(pTimer);
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

	pTimer->m_InExec = true; /* The timer is not really executed but this check needs to be done */
	pTimer->m_Listener->OnTimerEnd(pTimer, pTimer->m_pData);

	if (pTimer->m_Flags & TIMER_FLAG_REPEAT)
	{
        List<ITimer*> timerList = pTimer->m_Flags & TIMER_FLAG_TICK_PRECISE ? m_HighSpeedLoopTimers : m_LowSpeedLoopTimers; 
		timerList.remove(pTimer);
	} else {
		m_SingleTimers.remove(pTimer);
	}

	m_FreeTimers.push(pTimer);
}

CStack<ITimer *> s_tokill;
void TimerSystem::RemoveMapChangeTimers()
{
    const auto KillMapchangeTimers = [](List<ITimer*>& timerList) {
        for (ITimer* pTimer : timerList)
        {
            if (pTimer->m_Flags & TIMER_FLAG_NO_MAPCHANGE)
            {
                s_tokill.push(pTimer);
            }
        }
    };

    KillMapchangeTimers(m_SingleTimers);

    KillMapchangeTimers(m_LowSpeedLoopTimers);
    KillMapchangeTimers(m_HighSpeedLoopTimers);

	while (!s_tokill.empty())
	{
		KillTimer(s_tokill.front());
		s_tokill.pop();
	}
}

IMapTimer *TimerSystem::SetMapTimer(IMapTimer *pTimer)
{
	IMapTimer *old = m_pMapTimer;

	m_pMapTimer = pTimer;

	if (m_pMapTimer)
	{
		m_pMapTimer->SetMapTimerStatus(true);
	}

	if (old)
	{
		old->SetMapTimerStatus(false);
	}

	return old;
}

IMapTimer *TimerSystem::GetMapTimer()
{
	return m_pMapTimer;
}

void TimerSystem::MapTimeLeftChanged()
{
	m_pOnMapTimeLeftChanged->Execute(NULL);
}

void TimerSystem::NotifyOfGameStart(float offset)
{
	g_fGameStartTime = gpGlobals->curtime + offset;
}

float TimerSystem::GetTickedTime()
{
	return g_fUniversalTime;
}

bool TimerSystem::GetMapTimeLeft(float *time_left)
{
	if (!m_pMapTimer)
	{
		return false;
	}

	int time_limit;
	if (!m_bHasMapSimulatedYet || (time_limit = m_pMapTimer->GetMapTimeLimit()) < 1)
	{
		*time_left = -1.0f;
	}
	else
	{
		*time_left = (g_fGameStartTime + time_limit * 60.0f) - gpGlobals->curtime;
	}

	return true;
}
