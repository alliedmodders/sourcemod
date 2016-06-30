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
#ifndef _INCLUDE_SOURCEMOD_CONSOLE_DETOURS_H_
#define _INCLUDE_SOURCEMOD_CONSOLE_DETOURS_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <IForwardSys.h>
#include <sm_stringhashmap.h>

namespace SourceMod {
class ICommandArgs;
} // namespace SourceMod

class ConsoleDetours :
	public SMGlobalClass,
	public IFeatureProvider
{
	friend class PlayerManager;
	friend class GenericCommandHooker;

public:
	ConsoleDetours();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IFeatureProvider
	FeatureStatus GetFeatureStatus(FeatureType type, const char *name);
public:
	bool AddListener(IPluginFunction *fun, const char *command);
	bool RemoveListener(IPluginFunction *fun, const char *command);
private:
	cell_t InternalDispatch(int client, const SourceMod::ICommandArgs *args);
#if SOURCE_ENGINE >= SE_ORANGEBOX
	static cell_t Dispatch(ConCommand *pBase, const CCommand& args);
#else
	static cell_t Dispatch(ConCommand *pBase);
#endif
public:
	FeatureStatus GetStatus();
	bool IsEnabled()
	{
		return status == FeatureStatus_Available;
	}
private:
	FeatureStatus status;
	IChangeableForward *m_pForward;
	StringHashMap<IChangeableForward *> m_Listeners;
};

extern ConsoleDetours g_ConsoleDetours;

#endif /* _INCLUDE_SOURCEMOD_CONSOLE_DETOURS_H_ */

