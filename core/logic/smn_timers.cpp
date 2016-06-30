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

#include "common_logic.h"
#include <string.h>
#include <IHandleSys.h>
#include <ITimerSystem.h>
#include <IPluginSys.h>
#include <sh_stack.h>
#include "DebugReporter.h"
#include <bridge/include/CoreProvider.h>

using namespace SourceHook;

#define TIMER_DATA_HNDL_CLOSE	(1<<9)
#define TIMER_HNDL_CLOSE		(1<<9)

HandleType_t g_TimerType;

struct TimerInfo 
{
	ITimer *Timer;
	IPluginFunction *Hook;
	IPluginContext *pContext;
	Handle_t TimerHandle;
	int UserData;
	int Flags;
};

class TimerNatives :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public ITimedEvent
{
public:
	~TimerNatives();
public: //ITimedEvent
	ResultType OnTimer(ITimer *pTimer, void *pData);
	void OnTimerEnd(ITimer *pTimer, void *pData);
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public:
	TimerInfo *CreateTimerInfo();
	void DeleteTimerInfo(TimerInfo *pInfo);
private:
	CStack<TimerInfo *> m_FreeTimers;
};

TimerNatives::~TimerNatives()
{
	CStack<TimerInfo *>::iterator iter;
	for (iter=m_FreeTimers.begin(); iter!=m_FreeTimers.end(); iter++)
	{
		delete (*iter);
	}
	m_FreeTimers.popall();
}

void TimerNatives::OnSourceModAllInitialized()
{
	HandleAccess sec;

	handlesys->InitAccessDefaults(NULL, &sec);
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;

	g_TimerType = handlesys->CreateType("Timer", this, 0, NULL, &sec, g_pCoreIdent, NULL);
}

void TimerNatives::OnSourceModShutdown()
{
	handlesys->RemoveType(g_TimerType, g_pCoreIdent);
	g_TimerType = 0;
}

void TimerNatives::OnHandleDestroy(HandleType_t type, void *object)
{
	TimerInfo *pTimer = reinterpret_cast<TimerInfo *>(object);

	timersys->KillTimer(pTimer->Timer);
}

TimerInfo *TimerNatives::CreateTimerInfo()
{
	TimerInfo *pInfo;

	if (m_FreeTimers.empty())
	{
		pInfo = new TimerInfo;
	} else {
		pInfo = m_FreeTimers.front();
		m_FreeTimers.pop();
	}

	return pInfo;
}

void TimerNatives::DeleteTimerInfo(TimerInfo *pInfo)
{
	m_FreeTimers.push(pInfo);
}

ResultType TimerNatives::OnTimer(ITimer *pTimer, void *pData)
{
	TimerInfo *pInfo = reinterpret_cast<TimerInfo *>(pData);
	IPluginFunction *pFunc = pInfo->Hook;
	if (!pFunc->IsRunnable())
		return Pl_Continue;

	cell_t res = static_cast<cell_t>(Pl_Continue);

	pFunc->PushCell(pInfo->TimerHandle);
	pFunc->PushCell(pInfo->UserData);
	pFunc->Execute(&res);

	return static_cast<ResultType>(res);
}

void TimerNatives::OnTimerEnd(ITimer *pTimer, void *pData)
{
	HandleSecurity sec;
	HandleError herr;
	TimerInfo *pInfo = reinterpret_cast<TimerInfo *>(pData);
	Handle_t usrhndl = static_cast<Handle_t>(pInfo->UserData);

	sec.pOwner = pInfo->pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if (pInfo->Flags & TIMER_DATA_HNDL_CLOSE)
	{
		if ((herr=handlesys->FreeHandle(usrhndl, &sec)) != HandleError_None)
		{
			g_DbgReporter.GenerateError(pInfo->pContext, pInfo->Hook->GetFunctionID(), 
							  	 SP_ERROR_NATIVE, 
							  	 "Invalid data handle %x (error %d) passed during timer end with TIMER_DATA_HNDL_CLOSE",
							  	 usrhndl, herr);
		}
	}

	if (pInfo->TimerHandle != BAD_HANDLE)
	{
		if ((herr=handlesys->FreeHandle(pInfo->TimerHandle, &sec)) != HandleError_None)
		{
			g_DbgReporter.GenerateError(pInfo->pContext, pInfo->Hook->GetFunctionID(), 
				SP_ERROR_NATIVE, 
				"Invalid timer handle %x (error %d) during timer end, displayed function is timer callback, not the stack trace", 
				pInfo->TimerHandle, herr);
		}
	}

	DeleteTimerInfo(pInfo);
}

/*******************************
*                              *
* TIMER NATIVE IMPLEMENTATIONS *
*                              *
********************************/

static TimerNatives s_TimerNatives;

static cell_t smn_CreateTimer(IPluginContext *pCtx, const cell_t *params)
{
	IPluginFunction *pFunc;
	TimerInfo *pInfo;
	ITimer *pTimer;
	Handle_t hndl;
	int flags = params[4];

	pFunc = pCtx->GetFunctionById(params[2]);
	if (!pFunc)
	{
		return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	pInfo = s_TimerNatives.CreateTimerInfo();
	pTimer = timersys->CreateTimer(&s_TimerNatives, sp_ctof(params[1]), pInfo, flags);

	if (!pTimer)
	{
		s_TimerNatives.DeleteTimerInfo(pInfo);
		return 0;
	}

	pInfo->UserData = params[3];
	pInfo->Flags = flags;
	pInfo->Hook = pFunc;
	pInfo->Timer = pTimer;
	pInfo->pContext = pCtx;

	hndl = handlesys->CreateHandle(g_TimerType, pInfo, pCtx->GetIdentity(), g_pCoreIdent, NULL);

	/* If we can't get a handle, the timer isn't refcounted against the plugin and 
	 * we need to bail out to prevent a crash.
	 */
	if (hndl == BAD_HANDLE)
	{
		pInfo->TimerHandle = BAD_HANDLE;
		timersys->KillTimer(pTimer);

		return pCtx->ThrowNativeError("Could not create timer, no more handles");
	}

	pInfo->TimerHandle = hndl;

	return hndl;
}

static cell_t smn_KillTimer(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	TimerInfo *pInfo;

	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_TimerType, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid timer handle %x (error %d)", hndl, herr);
	}

	timersys->KillTimer(pInfo->Timer);

	if (params[2] && !(pInfo->Flags & TIMER_DATA_HNDL_CLOSE))
	{
		sec.pOwner = pInfo->pContext->GetIdentity();
		sec.pIdentity = g_pCoreIdent;

		if ((herr=handlesys->FreeHandle(static_cast<Handle_t>(pInfo->UserData), &sec)) != HandleError_None)
		{
			return pCtx->ThrowNativeError("Invalid data handle %x (error %d) on timer kill with TIMER_DATA_HNDL_CLOSE", hndl, herr);
		}
	}

	return 1;
}

static cell_t smn_TriggerTimer(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	TimerInfo *pInfo;

	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_TimerType, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid timer handle %x (error %d)", hndl, herr);
	}

	timersys->FireTimerOnce(pInfo->Timer, params[2] ? true : false);

	return 1;
}

static cell_t smn_GetTickedTime(IPluginContext *pContext, const cell_t *params)
{
	return sp_ftoc(*serverGlobals.universalTime);
}

static cell_t smn_GetMapTimeLeft(IPluginContext *pContext, const cell_t *params)
{
	float time_left;
	if (!timersys->GetMapTimeLeft(&time_left))
	{
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	*addr = (int)time_left;

	return 1;
}

static cell_t smn_GetMapTimeLimit(IPluginContext *pContext, const cell_t *params)
{
	IMapTimer *pMapTimer = timersys->GetMapTimer();

	if (!pMapTimer)
	{
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	*addr = pMapTimer->GetMapTimeLimit();

	return 1;
}

static cell_t smn_ExtendMapTimeLimit(IPluginContext *pContext, const cell_t *params)
{
	IMapTimer *pMapTimer = timersys->GetMapTimer();

	if (!pMapTimer)
	{
		return 0;
	}

	pMapTimer->ExtendMapTimeLimit(params[1]);

	return 1;
}

static cell_t smn_IsServerProcessing(IPluginContext *pContext, const cell_t *params)
{
	return (*serverGlobals.frametime > 0.0f) ? 1 : 0;
}

static cell_t smn_GetTickInterval(IPluginContext *pContext, const cell_t *params)
{
	return sp_ftoc(*serverGlobals.interval_per_tick);
}

REGISTER_NATIVES(timernatives)
{
	{"CreateTimer",				smn_CreateTimer},
	{"KillTimer",				smn_KillTimer},
	{"TriggerTimer",			smn_TriggerTimer},
	{"GetTickedTime",			smn_GetTickedTime},
	{"GetMapTimeLeft",			smn_GetMapTimeLeft},
	{"GetMapTimeLimit",			smn_GetMapTimeLimit},
	{"ExtendMapTimeLimit",		smn_ExtendMapTimeLimit},
	{"IsServerProcessing",		smn_IsServerProcessing},
	{"GetTickInterval",			smn_GetTickInterval},
	{NULL,						NULL}
};

