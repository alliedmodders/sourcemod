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
#include "ShareSys.h"

#if defined PLATFORM_LINUX
# include <dlfcn.h>
# include <sys/mman.h>
# include <stdint.h>
# include <unistd.h>
#endif

#if SH_IMPL_VERSION >= 5
# if SOURCE_ENGINE >= SE_ORANGEBOX
	SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
# else
	SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
# endif
#else
# if SH_IMPL_VERSION >= 4
 extern int __SourceHook_FHVPAddConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>,bool);
 extern int __SourceHook_FHAddConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
# else
 extern bool __SourceHook_FHAddConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
# endif
extern bool __SourceHook_FHRemoveConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
#endif

#if SH_IMPL_VERSION >= 4

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

# if SOURCE_ENGINE >= SE_ORANGEBOX
	void Dispatch(const CCommand& args)
# else
	void Dispatch()
# endif
	{
		cell_t res = ConsoleDetours::Dispatch(META_IFACEPTR(ConCommand)
# if SOURCE_ENGINE >= SE_ORANGEBOX
			, args
# endif
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
			g_Logger.LogError("Console detour tried to unhook command \"%s\" but it wasn't found", pBase->GetName());
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
			g_Logger.LogError("Command filter could not determine ConCommand layout");
			return false;
		}

		for (ConCommandBaseIterator iter; iter.IsValid(); iter.Next())
			MakeHookable(iter.Get());

		if (!vtables.size())
		{
			g_Logger.LogError("Command filter could not find any cvars!");
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
 * END DVP HOOK VERSION
 */

#else /* SH_IMPL_VERSION >= 4 */

/**
 * BEGIN ENGINE DETOUR VERSION
 */

# include <jit/jit_helpers.h>
# include <jit/x86/x86_macros.h>

class GenericCommandHooker
{
	struct Patch
	{
		Patch() : applied(false)
		{
		}

		bool applied;
		void *base;
		unsigned char search[16];
		size_t search_len;
		size_t offset;
		size_t saveat;
		int regparam;
		void *detour;
	};
	
	Patch ces;
	Patch cgc;

public:
 	static void DelayedActivation(void *inparam)
 	{
 		GenericCommandHooker *cdtrs = reinterpret_cast<GenericCommandHooker*>(inparam);
 		/* Safe to re-enter because the frame queue is lock+swapped. */
 		if ((!cdtrs->ces.applied || !cdtrs->cgc.applied) &&
 		    g_HL2.PeekCommandStack() != NULL)
 		{
 			g_SourceMod.AddFrameAction(GenericCommandHooker::DelayedActivation, cdtrs);
 			return;
 		}
 
 		if (!cdtrs->ces.applied)
 			cdtrs->ApplyPatch(&cdtrs->ces);
 		if (!cdtrs->cgc.applied)
 			cdtrs->ApplyPatch(&cdtrs->cgc);
 	}

	bool Enable()
	{
		const char *platform = NULL;
# if defined(PLATFORM_WINDOWS)
		platform = "Windows";
# else	
		void *addrInBase = (void *)g_SMAPI->GetEngineFactory(false);
		Dl_info info;
		if (!dladdr(addrInBase, &info))
		{
			g_Logger.LogError("Command filter could not find engine name");
			return false;
		}
		if (strstr(info.dli_fname, "engine_i486"))
		{
			platform = "Linux_486";
		}
		else if (strstr(info.dli_fname, "engine_i686"))
		{
			platform = "Linux_686";
		}
		else if (strstr(info.dli_fname, "engine_amd"))
		{
			platform = "Linux_AMD";
		}
		else
		{
			g_Logger.LogError("Command filter could not determine engine (%s)", info.dli_fname);
			return false;
		}
# endif

		if (!PrepPatch("Cmd_ExecuteString", "CES", platform, &ces))
			return false;
		if (!PrepPatch("CGameClient::ExecuteString", "CGC", platform, &cgc))
			return false;

 		if (g_HL2.PeekCommandStack() != NULL)
 		{
 			g_SourceMod.AddFrameAction(GenericCommandHooker::DelayedActivation, this);
 			return true;
 		}

		ApplyPatch(&ces);
		ApplyPatch(&cgc);

		return true;
	}

	void Disable()
	{
		UndoPatch(&ces);
		UndoPatch(&cgc);
	}

private:

# if !defined PLATFORM_WINDOWS
#  define PAGE_READWRITE     PROT_READ|PROT_WRITE
#  define PAGE_EXECUTE_READ  PROT_READ|PROT_EXEC

	static inline uintptr_t AddrToPage(uintptr_t address)
	{
		return (address & ~(uintptr_t(sysconf(_SC_PAGE_SIZE) - 1)));
	}

# endif

	void Protect(void *addr, size_t length, int prot)
	{
# if defined PLATFORM_WINDOWS
		DWORD ignore;
		VirtualProtect(addr, length, prot, &ignore);
# else
		uintptr_t startPage = AddrToPage(uintptr_t(addr));
		length += (uintptr_t(addr) - startPage);
		mprotect((void*)startPage, length, prot);
# endif
	}

	void UndoPatch(Patch *patch)
	{
		if (!patch->applied || patch->detour == NULL)
			return;

		g_pSourcePawn->FreePageMemory(patch->detour);

		unsigned char *source = (unsigned char *)patch->base + patch->offset;
		Protect(source, patch->search_len, PAGE_READWRITE);
		for (size_t i = 0; i < patch->search_len; i++)
			source[i] = patch->search[i];
		Protect(source, patch->search_len, PAGE_EXECUTE_READ);
	}

	void ApplyPatch(Patch *patch)
	{
		assert(!patch->applied);

		size_t length = 0;
		void *callback = (void*)&ConsoleDetours::Dispatch;

		/* Bogus assignment to make compiler is doing the right thing. */
		patch->detour = callback;

		/* Assemgle the detour. */
		JitWriter writer;
		writer.outbase = NULL;
		writer.outptr = NULL;
		do
		{
			/* Need a specific register, or value on stack? */
			if (patch->regparam != -1)
				IA32_Push_Reg(&writer, patch->regparam);
			/* Call real function. */
			IA32_Write_Jump32_Abs(&writer, IA32_Call_Imm32(&writer, 0), callback);
			/* Restore stack. */
			if (patch->regparam != -1)
				IA32_Pop_Reg(&writer, patch->regparam);
			/* Copy any saved bytes */
			if (patch->saveat)
			{
				for (size_t i = patch->saveat; i < patch->search_len; i++)
				{
					writer.write_ubyte(patch->search[i]);
				}
			}
			/* Jump back to the caller. */
			unsigned char *target = (unsigned char *)patch->base + patch->offset + patch->search_len;
			IA32_Jump_Imm32_Abs(&writer, target);
			/* Assemble, if we can. */
			if (writer.outbase == NULL)
			{
				length = writer.outptr - writer.outbase;
				patch->detour = g_pSourcePawn->AllocatePageMemory(length);
				if (patch->detour == NULL)
				{
					g_Logger.LogError("Ran out of memory!");
					return;
				}
				g_pSourcePawn->SetReadWrite(patch->detour);
				writer.outbase = (jitcode_t)patch->detour;
				writer.outptr = writer.outbase;
			}
			else
			{
				break;
			}
		} while (true);

		g_pSourcePawn->SetReadExecute(patch->detour);

		unsigned char *source = (unsigned char *)patch->base + patch->offset;
		Protect(source, 6, PAGE_READWRITE);
		source[0] = 0xFF;
		source[1] = 0x25;
		*(void **)&source[2] = &patch->detour;
		Protect(source, 6, PAGE_EXECUTE_READ);

		patch->applied = true;
	}

	bool PrepPatch(const char *signature, const char *name, const char *platform, Patch *patch)
	{
		/* Get the base address of the function. */
		if (!g_pGameConf->GetMemSig(signature, &patch->base) || patch->base == NULL)
		{
			g_Logger.LogError("Command filter could not find signature: %s", signature);
			return false;
		}

		const char *value;
		char keyname[255];

		/* Get the verification bytes that will be written over. */
		UTIL_Format(keyname, sizeof(keyname), "%s_Patch_%s", name, platform);
		if ((value = g_pGameConf->GetKeyValue(keyname)) == NULL)
		{
			g_Logger.LogError("Command filter could not find key: %s", keyname);
			return false;
		}
		patch->search_len = UTIL_DecodeHexString(patch->search, sizeof(patch->search), value);
		if (patch->search_len < 6)
		{
			g_Logger.LogError("Error decoding %s value, or not long enough", keyname);
			return false;
		}

		/* Get the offset into the function. */
		UTIL_Format(keyname, sizeof(keyname), "%s_Offset_%s", name, platform);
		if ((value = g_pGameConf->GetKeyValue(keyname)) == NULL)
		{
			g_Logger.LogError("Command filter could not find key: %s", keyname);
			return false;
		}
		patch->offset = atoi(value);
		if (patch->offset > 20000)
		{
			g_Logger.LogError("Command filter %s value is bogus", keyname);
			return false;
		}

		/* Get the number of bytes to save from what was written over. */
		patch->saveat = 0;
		UTIL_Format(keyname, sizeof(keyname), "%s_Save_%s", name, platform);
		if ((value = g_pGameConf->GetKeyValue(keyname)) != NULL)
		{
			patch->saveat = atoi(value);
			if (patch->saveat >= patch->search_len)
			{
				g_Logger.LogError("Command filter %s value is too large", keyname);
				return false;
			}
		}

		/* Get register for parameter, if any. */
		patch->regparam = -1;
		UTIL_Format(keyname, sizeof(keyname), "%s_Reg_%s", name, platform);
		if ((value = g_pGameConf->GetKeyValue(keyname)) != NULL)
		{
			patch->regparam = atoi(value);
		}

		/* Everything loaded from gamedata, make sure the patch will succeed. */
		unsigned char *address = (unsigned char *)patch->base + patch->offset;
		for (size_t i = 0; i < patch->search_len; i++)
		{
			if (address[i] != patch->search[i])
			{
				g_Logger.LogError("Command filter %s has changed (byte %x is not %x sub-offset %d)",
				                  name, address[i], patch->search[i], i);
				return false;
			}
		}

		return true;
	}
};

static bool dummy_hook_set = false;
void DummyHook()
{
	if (dummy_hook_set)
	{
		dummy_hook_set = false;
		RETURN_META(MRES_SUPERCEDE);
	}
}

#endif

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
	m_pForward = g_Forwards.CreateForwardEx("OnAnyCommand", ET_Hook, 3, NULL, Param_Cell,
	                                        Param_String, Param_Cell);
	g_ShareSys.AddCapabilityProvider(NULL, this, FEATURECAP_COMMANDLISTENER);
}

void ConsoleDetours::OnSourceModShutdown()
{
	List<Listener*>::iterator iter = m_Listeners.begin();
	while (iter != m_Listeners.end())
	{
		Listener *listener = (*iter);
		g_Forwards.ReleaseForward(listener->forward);
		delete listener;
		iter = m_Listeners.erase(iter);
	}

	g_Forwards.ReleaseForward(m_pForward);
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
		const char *str = UTIL_ToLowerCase(command);
		Listener *listener;
		Listener **plistener = m_CmdLookup.retrieve(str);
		if (plistener == NULL)
		{
			listener = new Listener;
			listener->forward = g_Forwards.CreateForwardEx(NULL, ET_Hook, 3, NULL, Param_Cell,
			                                               Param_String, Param_Cell);
			m_CmdLookup.insert(str, listener);
		}
		else
		{
			listener = *plistener;
		}
		listener->forward->AddFunction(fun);
		delete [] str;
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
		const char *str = UTIL_ToLowerCase(command);
		Listener *listener;
		Listener **plistener = m_CmdLookup.retrieve(str);
		delete [] str;
		if (plistener == NULL)
			return false;
		listener = *plistener;
		return listener->forward->RemoveFunction(fun);
	}
}

cell_t ConsoleDetours::InternalDispatch(int client, const CCommand& args)
{
	char name[255];
	const char *realname = args.Arg(0);
	size_t len = strlen(realname);
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
	m_pForward->PushCell(args.ArgC() - 1);
	m_pForward->Execute(&result, NULL);

	/* Don't let plugins block this. */
	if (strcmp(name, "sm") == 0)
		result = Pl_Continue;

	if (result >= Pl_Stop)
		return result;
	
	Listener **plistener = m_CmdLookup.retrieve(name);
	if (plistener == NULL)
		return result;
	Listener *listener = *plistener;
	if (listener->forward->GetFunctionCount() == 0)
		return result;

	cell_t result2 = Pl_Continue;
	listener->forward->PushCell(client);
	listener->forward->PushString(name);
	listener->forward->PushCell(args.ArgC() - 1);
	listener->forward->Execute(&result2, NULL);

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
	g_HL2.PushCommandStack(&args);
	cell_t res = g_ConsoleDetours.InternalDispatch(g_ConCmds.GetCommandClient(), args);
	g_HL2.PopCommandStack();

#if SH_IMPL_VERSION < 4
	if (res >= Pl_Handled)
	{
		/* See bug 4020 - we can't optimize this because looking at the vtable
		 * is probably more expensive, since there's no SH_GET_ORIG_VFNPTR_ENTRY.
		 */
		SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pBase, DummyHook, false);
		dummy_hook_set = true;
		pBase->Dispatch();
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pBase, DummyHook, false);
	}
	else
	{
		/* Make sure the command gets invoked. See bug 4019 on making this better. */
		pBase->Dispatch();
	}
#endif

	return res;
}
