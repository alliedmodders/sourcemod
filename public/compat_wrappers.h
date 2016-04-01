/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_
#define _INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_

#include <datamap.h>

#if SOURCE_ENGINE == SE_DARKMESSIAH
class EQueryCvarValueStatus;
typedef int QueryCvarCookie_t;
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
	#define CONVAR_REGISTER(object)				ConVar_Register(0, object)

	inline bool IsFlagSet(ConCommandBase *cmd, int flag)
	{
		return cmd->IsFlagSet(flag);
	}
	inline ConCommandBase *FindCommandBase(const char *name)
	{
		return icvar->FindCommandBase(name);
	}
	inline ConCommand *FindCommand(const char *name)
	{
		return icvar->FindCommand(name);
	}
#else
	class CCommand
	{
	public:
		inline const char *ArgS() const
		{
			return engine->Cmd_Args();
		}
		inline int ArgC() const
		{
			return engine->Cmd_Argc();
		}
		inline const char *Arg(int index) const
		{
			return engine->Cmd_Argv(index);
		}

		static int MaxCommandLength() { return 512; }
	};

	inline bool IsFlagSet(ConCommandBase *cmd, int flag)
	{
		return cmd->IsBitSet(flag);
	}
	inline ConCommandBase *FindCommandBase(const char *name)
	{
		ConCommandBase *pBase = icvar->GetCommands();
		while (pBase)
		{
			if (strcmp(pBase->GetName(), name) == 0)
			{
				if (!pBase->IsCommand())
				{
					return NULL;
				}

				return pBase;
			}
			pBase = const_cast<ConCommandBase *>(pBase->GetNext());
		}
		return NULL;
	}
	inline ConCommand *FindCommand(const char *name)
	{
		ConCommandBase *pBase = icvar->GetCommands();
		while (pBase)
		{
			if (strcmp(pBase->GetName(), name) == 0)
			{
				if (!pBase->IsCommand())
				{
					return NULL;
				}

				return static_cast<ConCommand *>(pBase);
			}
			pBase = const_cast<ConCommandBase *>(pBase->GetNext());
		}
		return NULL;
	}

	#define CVAR_INTERFACE_VERSION				VENGINE_CVAR_INTERFACE_VERSION

	#define CONVAR_REGISTER(object)				ConCommandBaseMgr::OneTimeInit(object)
	typedef FnChangeCallback					FnChangeCallback_t;
#endif //SOURCE_ENGINE >= SE_ORANGEBOX

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	inline int IndexOfEdict(const edict_t *pEdict)
	{
		return (int)(pEdict - gpGlobals->pEdicts);
	}
	inline edict_t *PEntityOfEntIndex(int iEntIndex)
	{
		if (iEntIndex >= 0 && iEntIndex < gpGlobals->maxEntities)
		{
			return (edict_t *)(gpGlobals->pEdicts + iEntIndex);
		}
		return NULL;
	}
	inline int GetTypeDescOffs(typedescription_t *td)
	{
		return td->fieldOffset;
	}
#else
	inline int IndexOfEdict(const edict_t *pEdict)
	{
		return engine->IndexOfEdict(pEdict);
	}
	inline edict_t *PEntityOfEntIndex(int iEntIndex)
	{
		return engine->PEntityOfEntIndex(iEntIndex);
	}
	inline int GetTypeDescOffs(typedescription_t *td)
	{
		return td->fieldOffset[TD_OFFSET_NORMAL];
	}
#endif //SOURCE_ENGINE >= SE_LEFT4DEAD

#endif //_INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_
