/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Counter-Strike:Source Extension
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

#include "extension.h"
#include "RegNatives.h"
#include "timeleft.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);

CStrike g_CStrike;		/**< Global singleton for extension's main interface */
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;
IGameEventManager2 *gameevents = NULL;
bool hooked_everything = false;

SMEXT_LINK(&g_CStrike);

extern sp_nativeinfo_t g_CSNatives[];

bool CStrike::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile("sm-cstrike.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (error)
		{
			snprintf(error, maxlength, "Could not read sm-cstrike.games.txt: %s", conf_error);
		}
		return false;
	}

	sharesys->AddNatives(myself, g_CSNatives);
	sharesys->RegisterLibrary(myself, "cstrike");

	return true;
}

bool CStrike::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	return true;
}

void CStrike::SDK_OnUnload()
{
	if (hooked_everything)
	{
		gameevents->RemoveListener(&g_TimeLeftEvents);
		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, &g_TimeLeftEvents, &TimeLeftEvents::LevelInit, true);
		hooked_everything = false;
	}
	g_RegNatives.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void CStrike::SDK_OnAllLoaded()
{
	gameevents->AddListener(&g_TimeLeftEvents, "round_start", true);
	gameevents->AddListener(&g_TimeLeftEvents, "round_end", true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, &g_TimeLeftEvents, &TimeLeftEvents::LevelInit, true);
	hooked_everything = true;

	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
}

bool CStrike::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool CStrike::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void CStrike::NotifyInterfaceDrop(SMInterface *pInterface)
{
	g_RegNatives.UnregisterAll();
}
