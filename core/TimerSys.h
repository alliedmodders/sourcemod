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

struct TickInfo
{
	bool ticking;				/* true=game is ticking, false=we're ticking */
	unsigned int tickcount;		/* number of simulated ticks we've done */
	float ticktime;				/* tick time we're maintaining */
};

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
	~TimerSystem();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
public: //ITimerSystem
	ITimer *CreateTimer(ITimedEvent *pCallbacks, float fInterval, void *pData, int flags);
	void KillTimer(ITimer *pTimer);
	void FireTimerOnce(ITimer *pTimer, bool delayExec=false);
public:
	void RunFrame();
	void MapChange();
private:
	List<ITimer *> m_SingleTimers;
	List<ITimer *> m_LoopTimers;
	CStack<ITimer *> m_FreeTimers;
	float m_LastExecTime;
};

extern TimerSystem g_Timers;
extern TickInfo g_SimTicks;

#endif //_INCLUDE_SOURCEMOD_CTIMERSYS_H_
