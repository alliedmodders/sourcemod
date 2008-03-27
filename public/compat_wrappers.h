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

#ifndef _INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_
#define _INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_

#if defined ORANGEBOX_BUILD
	#define CONVAR_REGISTER(object)				ConVar_Register(0, object)

	inline bool IsFlagSet(ConCommandBase *cmd, int flag)
	{
		return cmd->IsFlagSet(flag);
	}
	inline void InsertServerCommand(const char *buf)
	{
		engine->InsertServerCommand(buf);
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
	};

	inline bool IsFlagSet(ConCommandBase *cmd, int flag)
	{
		return cmd->IsBitSet(flag);
	}
	inline void InsertServerCommand(const char *buf)
	{
		engine->InsertServerCommand(buf);
	}

	#define CVAR_INTERFACE_VERSION				VENGINE_CVAR_INTERFACE_VERSION

	#define CONVAR_REGISTER(object)				ConCommandBaseMgr::OneTimeInit(object)
	typedef FnChangeCallback					FnChangeCallback_t;
#endif //ORANGEBOX_BUILD

#endif //_INCLUDE_SOURCEMOD_COMPAT_WRAPPERS_H_
