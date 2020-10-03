/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

/**
 * On SourceHook v4.3 or lower, there are no DVP hooks. Very sad, right?
 * Only do this on newer versions. For the older code, we'll do an incredibly
 * hacky detour.
 *
 * The idea of the "non-hacky" (yeah... no) code is that every unique
 * ConCommand vtable gets its own DVP hook. We watch for unloading and
 * loading commands to remove stale hooks from SH.
 */

#include "sourcemod.h"
#include "sourcemm_api.h"
#include "Logger.h"
#include "compat_wrappers.h"
#include "ConsoleDetours.h"
#include <IGameConfigs.h>
#include "sm_stringutil.h"
#include "ConCmdManager.h"
#include "HalfLife2.h"
#include "ConCommandBaseIterator.h"
#include "logic_bridge.h"
#include "command_args.h"
#include "provider.h"
#include <am-utility.h>
#include <bridge/include/ILogger.h>

#if defined PLATFORM_POSIX
# include <dlfcn.h>
# include <sys/mman.h>
# include <stdint.h>
# include <unistd.h>
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
	SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
	SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif

class GenericCommandHooker : public IConCommandLinkListener
{
	struct HackInfo
	{
		void **vtable;
		int hook;
		unsigned int refcount;
	};
	CVector<HackInfo> vtables;
	bool enabled;
	SourceHook::MemFuncInfo dispatch;

	inline void **GetVirtualTable(ConCommandBase *pBase)
	{
		return *reinterpret_cast<void***>(reinterpret_cast<char*>(pBase) +
		                                  dispatch.thisptroffs +
		                                  dispatch.vtbloffs);
	}

	inline bool FindVtable(void **ptr, size_t& index)
	{
		for (size_t i = 0; i < vtables.size(); i++)
		{
			if (vtables[i].vtable == ptr)
			{
				index = i;
				return true;
			}
		}
		return false;
	}

	void MakeHookable(ConCommandBase *pBase)
	{
		if (!pBase->IsCommand())
			return;

		ConCommand *cmd = (ConCommand*)pBase;
		void **vtable = GetVirtualTable(cmd);

		size_t index;
		if (!FindVtable(vtable, index))
		{
			HackInfo hack;
			hack.vtable = vtable;
			hack.hook = SH_ADD_VPHOOK(ConCommand, Dispatch, cmd, SH_MEMBER(this, &GenericCommandHooker::Dispatch), false);
			hack.refcount = 1;
			vtables.push_back(hack);
		}
		else
		{
			vtables[index].refcount++;
		}
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX
	void Dispatch(const CCommand& args)
#else
	void Dispatch()
#endif
	{
		cell_t res = ConsoleDetours::Dispatch(META_IFACEPTR(ConCommand)
#if SOURCE_ENGINE >= SE_ORANGEBOX
			, args
#endif
			);
		if (res >= Pl_Handled)
			RETURN_META(MRES_SUPERCEDE);
	}

	void ReparseCommandList()
	{
		for (size_t i = 0; i < vtables.size(); i++)
			vtables[i].refcount = 0;
		for (ConCommandBaseIterator iter; iter.IsValid(); iter.Next())
			MakeHookable(iter.Get());
		CVector<HackInfo>::iterator iter = vtables.begin();
		while (iter != vtables.end())
		{
			if ((*iter).refcount)
			{
				iter++;
				continue;
			}
			/* Damn it. This event happens AFTER the plugin has unloaded!
			 * There's two options. Remove the hook now and hope SH's memory
			 * protection will prevent a crash. Otherwise, we can wait until
			 * the server shuts down and more likely crash then.
			 *
			 * This situation only arises if:
			 * 1) Someone has used AddCommandFilter()
			 * 2) ... on a Dark Messiah server (mm:s new api)
			 * 3) ... and another MM:S plugin that uses ConCommands has unloaded.
			 *
			 * Even though the impact is really small, we'll wait until the
			 * server shuts down, so normal operation isn't interrupted.
			 *
			 * See bug 4018.
			 */
			iter = vtables.erase(iter);
		}
	}

	void UnhookCommand(ConCommandBase *pBase)
	{
		if (!pBase->IsCommand())
			return;

		ConCommand *cmd = (ConCommand*)pBase;
		void **vtable = GetVirtualTable(cmd);

		size_t index;
		if (!FindVtable(vtable, index))
		{
			logger->LogError("Console detour tried to unhook command \"%s\" but it wasn't found", pBase->GetName());
			return;
		}

		assert(vtables[index].refcount > 0);
		vtables[index].refcount--;
		if (vtables[index].refcount == 0)
		{
			SH_REMOVE_HOOK_ID(vtables[index].hook);
			vtables.erase(vtables.iterAt(index));
		}
	}

public:
	GenericCommandHooker() : enabled(false)
	{
	}

	bool Enable()
	{
		SourceHook::GetFuncInfo(&ConCommand::Dispatch, dispatch);

		if (dispatch.thisptroffs < 0)
		{
			logger->LogError("Command filter could not determine ConCommand layout");
			return false;
		}

		for (ConCommandBaseIterator iter; iter.IsValid(); iter.Next())
			MakeHookable(iter.Get());

		if (!vtables.size())
		{
			logger->LogError("Command filter could not find any cvars!");
			return false;
		}

		enabled = true;

		return true;
	}

	void Disable()
	{
		for (size_t i = 0; i < vtables.size(); i++)
			SH_REMOVE_HOOK_ID(vtables[i].hook);
		vtables.clear();
	}

	virtual void OnLinkConCommand(ConCommandBase *pBase)
	{
		if (!enabled)
			return;
		MakeHookable(pBase);
	}

	virtual void OnUnlinkConCommand(ConCommandBase *pBase)
	{
		if (!enabled)
			return;
		if (pBase == NULL)
			ReparseCommandList();
		else
			UnhookCommand(pBase);
	}
};

/**
 * BEGIN THE ACTUALLY GENERIC CODE.
 */

#define FEATURECAP_COMMANDLISTENER  "command listener"

static GenericCommandHooker s_GenericHooker;
ConsoleDetours g_ConsoleDetours;

ConsoleDetours::ConsoleDetours() : status(FeatureStatus_Unknown)
{
}

void ConsoleDetours::OnSourceModAllInitialized()
{
	m_pForward = forwardsys->CreateForwardEx("OnAnyCommand", ET_Hook, 3, NULL, Param_Cell,
	                                        Param_String, Param_Cell);
	sharesys->AddCapabilityProvider(NULL, this, FEATURECAP_COMMANDLISTENER);
}

void ConsoleDetours::OnSourceModShutdown()
{
	for (StringHashMap<IChangeableForward *>::iterator iter = m_Listeners.iter();
		 !iter.empty();
		 iter.next())
	{
		forwardsys->ReleaseForward(iter->value);
	}

	forwardsys->ReleaseForward(m_pForward);
	s_GenericHooker.Disable();
}

FeatureStatus ConsoleDetours::GetFeatureStatus(FeatureType type, const char *name)
{
	return GetStatus();
}

FeatureStatus ConsoleDetours::GetStatus()
{
	if (status == FeatureStatus_Unknown)
	{
		status = s_GenericHooker.Enable() ? FeatureStatus_Available : FeatureStatus_Unavailable;
	}
	return status;
}

bool ConsoleDetours::AddListener(IPluginFunction *fun, const char *command)
{
	if (GetStatus() != FeatureStatus_Available)
		return false;

	if (command == NULL)
	{
		m_pForward->AddFunction(fun);
	}
	else
	{
		std::unique_ptr<char[]> str(UTIL_ToLowerCase(command));
		IChangeableForward *forward;
		if (!m_Listeners.retrieve(str.get(), &forward))
		{
			forward = forwardsys->CreateForwardEx(NULL, ET_Hook, 3, NULL, Param_Cell,
			                                     Param_String, Param_Cell);
			m_Listeners.insert(str.get(), forward);
		}
		forward->AddFunction(fun);
	}

	return true;
}

bool ConsoleDetours::RemoveListener(IPluginFunction *fun, const char *command)
{
	if (command == NULL)
	{
		return m_pForward->RemoveFunction(fun);
	}
	else
	{
		std::unique_ptr<char[]> str(UTIL_ToLowerCase(command));
		IChangeableForward *forward;
		if (!m_Listeners.retrieve(str.get(), &forward))
			return false;
		return forward->RemoveFunction(fun);
	}
}

cell_t ConsoleDetours::InternalDispatch(int client, const ICommandArgs *args)
{
	char name[255];
	const char *realname = args->Arg(0);
	size_t len = strlen(realname);

	// Disallow command strings that are too long, for now.
	if (len >= sizeof(name) - 1)
		return Pl_Continue;

	for (size_t i = 0; i < len; i++)
	{
		if (realname[i] >= 'A' && realname[i] <= 'Z')
			name[i] = tolower(realname[i]);
		else
			name[i] = realname[i];
	}
	name[len] = '\0';

	cell_t result = Pl_Continue;
	m_pForward->PushCell(client);
	m_pForward->PushString(name);
	m_pForward->PushCell(args->ArgC() - 1);
	m_pForward->Execute(&result, NULL);

	/* Don't let plugins block this. */
	if (strcmp(name, "sm") == 0)
		result = Pl_Continue;

	if (result >= Pl_Handled)
		return result;
	
	IChangeableForward *forward;
	if (!m_Listeners.retrieve(name, &forward))
		return result;
	if (forward->GetFunctionCount() == 0)
		return result;

	cell_t result2 = Pl_Continue;
	forward->PushCell(client);
	forward->PushString(name);
	forward->PushCell(args->ArgC() - 1);
	forward->Execute(&result2, NULL);

	if (result2 > result)
		result = result2;

	/* "sm" should not have flown through the above. */
	assert(strcmp(name, "sm") != 0 || result == Pl_Continue);

	return result;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
cell_t ConsoleDetours::Dispatch(ConCommand *pBase, const CCommand& args)
{
#else
cell_t ConsoleDetours::Dispatch(ConCommand *pBase)
{
	CCommand args;
#endif
	EngineArgs cargs(args);
	cell_t res;
	{
		AutoEnterCommand autoEnterCommand(&cargs);
		res = g_ConsoleDetours.InternalDispatch(sCoreProviderImpl.CommandClient(), &cargs);
	}

	return res;
}
