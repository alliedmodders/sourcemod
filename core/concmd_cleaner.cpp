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

#include "sm_globals.h"
#include <sh_list.h>
#include <convar.h>
#include "concmd_cleaner.h"
#include "sm_stringutil.h"
#include "sourcemm_api.h"

#if defined ORANGEBOX_BUILD
SH_DECL_HOOK1_void(ICvar, UnregisterConCommand, SH_NOATTRIB, 0, ConCommandBase *);
#endif

using namespace SourceHook;

struct ConCommandInfo
{
	ConCommandBase *pBase;
	IConCommandTracker *cls;
	char name[64];
};

List<ConCommandInfo *> tracked_bases;

ConCommandBase *FindConCommandBase(const char *name);

class ConCommandCleaner : public SMGlobalClass
{
public:
#if defined ORANGEBOX_BUILD
	void OnSourceModAllInitialized()
	{
		SH_ADD_HOOK(ICvar, UnregisterConCommand, SH_NOATTRIB, SH_MEMBER(this, &ConCommandCleaner::UnlinkConCommandBase), false);
	}

	void OnSourceModShutdown()
	{
		SH_REMOVE_HOOK(ICvar, UnregisterConCommand, SH_NOATTRIB, SH_MEMBER(this, &ConCommandCleaner::UnlinkConCommandBase), false);
	}
#endif

	void UnlinkConCommandBase(ConCommandBase *pBase)
	{
		ConCommandInfo *pInfo;
		List<ConCommandInfo *>::iterator iter = tracked_bases.begin();

		if (pBase)
		{
			while (iter != tracked_bases.end())
			{
				if ((*iter)->pBase == pBase)
				{
					pInfo = (*iter);
					iter = tracked_bases.erase(iter);
					pInfo->cls->OnUnlinkConCommandBase(pBase, pBase->GetName(), true);
					delete pInfo;
				}
				else
				{
					iter++;
				}
			}
		}
		else
		{
			while (iter != tracked_bases.end())
			{
				/* This is just god-awful! */
				if (FindConCommandBase((*iter)->name) != (*iter)->pBase)
				{
					pInfo = (*iter);
					iter = tracked_bases.erase(iter);
					pInfo->cls->OnUnlinkConCommandBase(pBase, pInfo->name, true);
					delete pInfo;
				}
				else
				{
					iter++;
				}
			}
		}
	}

	void AddTarget(ConCommandBase *pBase, IConCommandTracker *cls)
	{
		ConCommandInfo *info = new ConCommandInfo;

		info->pBase = pBase;
		info->cls = cls;
		strncopy(info->name, pBase->GetName(), sizeof(info->name));

		tracked_bases.push_back(info);
	}

	void RemoveTarget(ConCommandBase *pBase, IConCommandTracker *cls)
	{
		List<ConCommandInfo *>::iterator iter;
		ConCommandInfo *pInfo;

		iter = tracked_bases.begin();
		while (iter != tracked_bases.end())
		{
			pInfo = (*iter);
			if (pInfo->pBase == pBase && pInfo->cls == cls)
			{
				delete pInfo;
				iter = tracked_bases.erase(iter);
			}
			else
			{
				iter++;
			}
		}
	}
} s_ConCmdTracker;

ConCommandBase *FindConCommandBase(const char *name)
{
	const ConCommandBase *pBase = icvar->GetCommands();

	while (pBase != NULL)
	{
		if (strcmp(pBase->GetName(), name) == 0)
		{
			return const_cast<ConCommandBase *>(pBase);
		}
		pBase = pBase->GetNext();
	}

	return NULL;
}

void TrackConCommandBase(ConCommandBase *pBase, IConCommandTracker *me)
{
	s_ConCmdTracker.AddTarget(pBase, me);
}

void UntrackConCommandBase(ConCommandBase *pBase, IConCommandTracker *me)
{
	s_ConCmdTracker.RemoveTarget(pBase, me);
}

void Global_OnUnlinkConCommandBase(ConCommandBase *pBase)
{
	s_ConCmdTracker.UnlinkConCommandBase(pBase);
}
