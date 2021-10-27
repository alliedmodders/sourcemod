// vim: set ts=4 sw=4 tw=99 et:
// =============================================================================
// SourceMod
// Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#include "sm_globals.h"
#include <sh_list.h>
#include <convar.h>
#include "concmd_cleaner.h"
#include "sm_stringutil.h"
#include "sourcemm_api.h"
#include "compat_wrappers.h"
#include <amtl/am-string.h>

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK1_void(ICvar, UnregisterConCommand, SH_NOATTRIB, 0, ConCommandBase *);
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
SH_DECL_HOOK2_void(ICvar, RegisterConCommand, SH_NOATTRIB, 0, ConCommandBase *, bool);
#else
SH_DECL_HOOK1_void(ICvar, RegisterConCommand, SH_NOATTRIB, 0, ConCommandBase *);
#endif
#else
SH_DECL_HOOK1_void(ICvar, RegisterConCommandBase, SH_NOATTRIB, 0, ConCommandBase *);
#endif

using namespace SourceHook;

struct ConCommandInfo
{
	ConCommandBase *pBase;
	IConCommandTracker *cls;
	char name[64];
};

List<ConCommandInfo *> tracked_bases;
IConCommandLinkListener *IConCommandLinkListener::head = NULL;

ConCommandBase *FindConCommandBase(const char *name);

class ConCommandCleaner : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
#if SOURCE_ENGINE >= SE_ORANGEBOX
		SH_ADD_HOOK(ICvar, UnregisterConCommand, icvar, SH_MEMBER(this, &ConCommandCleaner::UnlinkConCommandBase), false);
		SH_ADD_HOOK(ICvar, RegisterConCommand, icvar, SH_MEMBER(this, &ConCommandCleaner::LinkConCommandBase), false);
#else
		SH_ADD_HOOK(ICvar, RegisterConCommandBase, icvar, SH_MEMBER(this, &ConCommandCleaner::LinkConCommandBase), false);
#endif
	}

	void OnSourceModShutdown()
	{
#if SOURCE_ENGINE >= SE_ORANGEBOX
		SH_REMOVE_HOOK(ICvar, UnregisterConCommand, icvar, SH_MEMBER(this, &ConCommandCleaner::UnlinkConCommandBase), false);
		SH_REMOVE_HOOK(ICvar, RegisterConCommand, icvar, SH_MEMBER(this, &ConCommandCleaner::LinkConCommandBase), false);
#else
		SH_REMOVE_HOOK(ICvar, RegisterConCommandBase, icvar, SH_MEMBER(this, &ConCommandCleaner::LinkConCommandBase), false);
#endif
	}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	void LinkConCommandBase(ConCommandBase *pBase, bool unknown)
#else
	void LinkConCommandBase(ConCommandBase *pBase)
#endif
	{
		IConCommandLinkListener *listener = IConCommandLinkListener::head;
		while (listener)
		{
			listener->OnLinkConCommand(pBase);
			listener = listener->next;
		}
	}

	void UnlinkConCommandBase(ConCommandBase *pBase)
	{
		ConCommandInfo *pInfo;
		List<ConCommandInfo *>::iterator iter = tracked_bases.begin();

		IConCommandLinkListener *listener = IConCommandLinkListener::head;
		while (listener)
		{
			listener->OnUnlinkConCommand(pBase);
			listener = listener->next;
		}

		while (iter != tracked_bases.end())
		{
		    if ((*iter)->pBase == pBase)
		    {
			pInfo = (*iter);
			iter = tracked_bases.erase(iter);
			pInfo->cls->OnUnlinkConCommandBase(pBase, pBase->GetName());
			delete pInfo;
		    }
		    else
		    {
			iter++;
		    }
		}
	}

	void AddTarget(ConCommandBase *pBase, IConCommandTracker *cls)
	{
		ConCommandInfo *info = new ConCommandInfo;

		info->pBase = pBase;
		info->cls = cls;
		ke::SafeStrcpy(info->name, sizeof(info->name), pBase->GetName());

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
