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
#include "frame_hooks.h"
#include "TimerSys.h"
#include "HalfLife2.h"
#include "MenuStyle_Valve.h"
#include "MenuStyle_Radio.h"
#include "PlayerManager.h"
#include "CoreConfig.h"
#include <sm_queue.h>
#include <IThreader.h>
#include "sourcemod.h"

static IMutex *frame_mutex;
static Queue<FrameAction> *frame_queue;
static Queue<FrameAction> *frame_actions;
static float g_LastMenuTime = 0.0f;
static float g_LastAuthCheck = 0.0f;
bool g_PendingInternalPush = false;

class FrameActionInit : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		frame_queue = new Queue<FrameAction>();
		frame_actions = new Queue<FrameAction>();
		frame_mutex = g_pThreader->MakeMutex();
	}

	void OnSourceModShutdown()
	{
		delete frame_queue;
		delete frame_actions;
		frame_mutex->DestroyThis();
	}
} s_FrameActionInit;

void AddFrameAction(const FrameAction & action)
{
	frame_mutex->Lock();
	frame_queue->push(action);
	frame_mutex->Unlock();
}

void RunFrameHooks(bool simulating)
{
	/* It's okay if this check races. */
	if (frame_queue->size())
	{
		Queue<FrameAction> *temp;

		/* Very quick lock to move queue/actions back and forth */
		frame_mutex->Lock();
		temp = frame_queue;
		frame_queue = frame_actions;
		frame_actions = temp;
		frame_mutex->Unlock();

		/* The server will now be adding to the other queue, so we can process events. */
		while (!frame_actions->empty())
		{
			FrameAction &item = frame_actions->first();
			frame_actions->pop();
			item.action(item.data);
		}
	}

	/* Frame based hooks */
	g_HL2.ProcessFakeCliCmdQueue();
	g_HL2.ProcessDelayedKicks();

    if (g_PendingInternalPush)
    {
        SM_InternalCmdTrigger();
    }

	g_SourceMod.ProcessGameFrameHooks(simulating);

	float curtime = *g_pUniversalTime;

	if (curtime - g_LastMenuTime >= 1.0f)
	{
		g_ValveMenuStyle.ProcessWatchList();
		g_RadioMenuStyle.ProcessWatchList();
		g_LastMenuTime = curtime;
	}

	if (*g_NumPlayersToAuth && curtime - g_LastAuthCheck >= 0.7f)
	{
		g_Players.RunAuthChecks();
		g_LastAuthCheck = curtime;
	}
}
