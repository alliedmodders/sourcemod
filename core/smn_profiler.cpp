/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include "HandleSys.h"

//Note: Do not add this to Linux yet, i haven't done the HPET timing research (if even available)
//nonetheless we need accurate counting
#if defined PLATFORM_LINUX
#error "Not supported"
#endif

struct Profiler
{
	Profiler()
	{
		started = false;
		stopped = false;
	}
#if defined PLATFORM_WINDOWS
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	double freq;
#endif
	bool started;
	bool stopped;
};

HandleType_t g_ProfilerType = 0;

class ProfilerHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		g_ProfilerType = g_HandleSys.CreateType("Profiler", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_ProfilerType, g_pCoreIdent);
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		Profiler *prof = (Profiler *)object;
		delete prof;
	}
} s_ProfHelpers;

static cell_t CreateProfiler(IPluginContext *pContext, const cell_t *params)
{
	Profiler *p = new Profiler();

#if defined PLATFORM_WINDOWS
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	p->freq = 1.0 / (double)(qpf.QuadPart);
#endif

	Handle_t hndl = g_HandleSys.CreateHandle(g_ProfilerType, p, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		delete p;
	}

	return hndl;
}

static cell_t StartProfiling(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = params[1];
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);
	HandleError err;
	Profiler *prof;

	if ((err = g_HandleSys.ReadHandle(hndl, g_ProfilerType, &sec, (void **)&prof))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

#if defined PLATFORM_WINDOWS
	QueryPerformanceCounter(&prof->start);
#endif

	prof->started = true;
	prof->stopped = false;

	return 1;
}

static cell_t StopProfiling(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = params[1];
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);
	HandleError err;
	Profiler *prof;

	if ((err = g_HandleSys.ReadHandle(hndl, g_ProfilerType, &sec, (void **)&prof))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (!prof->started)
	{
		return pContext->ThrowNativeError("Profiler was never started");
	}

#if defined PLATFORM_WINDOWS
	QueryPerformanceCounter(&prof->end);
#endif

	prof->started = false;
	prof->stopped = true;

	return 1;
}

static cell_t GetProfilerTime(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = params[1];
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);
	HandleError err;
	Profiler *prof;

	if ((err = g_HandleSys.ReadHandle(hndl, g_ProfilerType, &sec, (void **)&prof))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (!prof->stopped)
	{
		return pContext->ThrowNativeError("Profiler was never stopped");
	}

	float fTime;

#if defined PLATFORM_WINDOWS
	LONGLONG diff = prof->end.QuadPart - prof->start.QuadPart;
	double seconds = diff * prof->freq;
	fTime = (float)seconds;
#endif

	return sp_ftoc(fTime);
}

REGISTER_NATIVES(profilerNatives)
{
	{"CreateProfiler",			CreateProfiler},
	{"GetProfilerTime",			GetProfilerTime},
	{"StartProfiling",			StartProfiling},
	{"StopProfiling",			StopProfiling},
	{NULL,						NULL},
};
