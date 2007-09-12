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

#ifndef _INCLUDE_SOURCEMOD_CTIMERSYS_H_
#define _INCLUDE_SOURCEMOD_CTIMERSYS_H_

#include "ShareSys.h"
#include <ITimerSystem.h>
#include <sh_stack.h>
#include <sh_list.h>
#include "sourcemm_api.h"

using namespace SourceHook;
using namespace SourceMod;

typedef List<ITimer *> TimerList;
typedef List<ITimer *>::iterator TimerIter;

class SourceMod::ITimer
{
public:
	void Initialize(ITimedEvent *pCallbacks, float fInterval, float fToExec, void *pData, int flags);
	ITimedEvent *m_Listener;
	void *m_pData;
	float m_Interval;
	float m_ToExec;
	int m_Flags;
	bool m_InExec;
	bool m_KillMe;
};

class TimerSystem : 
	public ITimerSystem,
	public SMGlobalClass
{
public:
	TimerSystem();
	~TimerSystem();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModLevelEnd();
	void OnSourceModShutdown();
public: //ITimerSystem
	ITimer *CreateTimer(ITimedEvent *pCallbacks, float fInterval, void *pData, int flags);
	void KillTimer(ITimer *pTimer);
	void FireTimerOnce(ITimer *pTimer, bool delayExec=false);
	SM_TIMELEFT_FUNCTION SetTimeLeftFunction(SM_TIMELEFT_FUNCTION fn);
public:
	void RunFrame();
	void MapChange(bool real_mapchange);
	void GameFrame(bool simulating);
private:
	List<ITimer *> m_SingleTimers;
	List<ITimer *> m_LoopTimers;
	List<ITimer *> m_MapEndTimers;
	CStack<ITimer *> m_FreeTimers;
	float m_LastExecTime;
	SM_TIMELEFT_FUNCTION m_fnTimeLeft;

	/* This is stuff for our manual ticking escapades. */
	bool m_bHasMapTickedYet;	/** Has the map ticked yet? */
	float m_fLastTickedTime;	/** Last time that the game currently gave 
									us while ticking.
									*/

	IForward *m_pOnGameFrame;
};

time_t GetAdjustedTime(time_t *buf = NULL);

extern const float *g_pUniversalTime;
extern TimerSystem g_Timers;

#endif //_INCLUDE_SOURCEMOD_CTIMERSYS_H_

