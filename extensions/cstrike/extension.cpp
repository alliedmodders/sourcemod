/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Counter-Strike:Source Extension
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

#include <sourcemod_version.h>
#include "extension.h"
#include "RegNatives.h"
#include "timeleft.h"
#include "iplayerinfo.h"
#include "ISDKTools.h"
#include "forwards.h"
#if SOURCE_ENGINE == SE_CSGO
#include "rulesfix.h"
#endif
#include "util_cstrike.h"

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
int g_msgHintText = -1;
CGlobalVars *gpGlobals;

SMEXT_LINK(&g_CStrike);

extern sp_nativeinfo_t g_CSNatives[];

ISDKTools *g_pSDKTools = NULL;

bool CStrike::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
#if SOURCE_ENGINE != SE_CSGO
	if (strcmp(g_pSM->GetGameFolderName(), "cstrike") != 0)
	{
		ke::SafeStrcpy(error, maxlength, "Cannot Load Cstrike Extension on mods other than CS:S and CS:GO");
		return false;
	}
#endif

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", false, true);

	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile("sm-cstrike.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (error)
		{
			ke::SafeSprintf(error, maxlength, "Could not read sm-cstrike.games: %s", conf_error);
		}
		return false;
	}

	sharesys->AddNatives(myself, g_CSNatives);
	sharesys->RegisterLibrary(myself, "cstrike");
	plsys->AddPluginsListener(this);

	playerhelpers->RegisterCommandTargetProcessor(this);

#if SOURCE_ENGINE == SE_CSGO
	rulesfix.OnLoad();
#endif

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

	g_pHandleBuyForward = forwards->CreateForward("CS_OnBuyCommand", ET_Event, 2, NULL, Param_Cell, Param_String);
	g_pPriceForward = forwards->CreateForward("CS_OnGetWeaponPrice", ET_Event, 3, NULL, Param_Cell, Param_String, Param_CellByRef);
	g_pTerminateRoundForward = forwards->CreateForward("CS_OnTerminateRound", ET_Event, 2, NULL, Param_FloatByRef, Param_CellByRef);
	g_pCSWeaponDropForward = forwards->CreateForward("CS_OnCSWeaponDrop", ET_Event, 3, NULL, Param_Cell, Param_Cell, Param_Cell);


	m_TerminateRoundDetourEnabled = false;
	m_WeaponPriceDetourEnabled = false;
	m_HandleBuyDetourEnabled = false;
	m_CSWeaponDetourEnabled = false;

	return true;
}

bool CStrike::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	gpGlobals = ismm->GetCGlobals();

#if SOURCE_ENGINE == SE_CSGO
	ICvar *icvar;
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	g_pCVar = icvar;
#endif

	return true;
}

void CStrike::SDK_OnUnload()
{
	if (hooked_everything)
	{
		gameevents->RemoveListener(&g_TimeLeftEvents);
		SH_REMOVE_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(&g_TimeLeftEvents, &TimeLeftEvents::LevelInit), true);
		hooked_everything = false;
	}

	if (m_TerminateRoundDetourEnabled)
	{
		RemoveTerminateRoundDetour();
		m_TerminateRoundDetourEnabled = false;
	}
	if (m_WeaponPriceDetourEnabled)
	{
		RemoveWeaponPriceDetour();
		m_WeaponPriceDetourEnabled = false;
	}
	if (m_HandleBuyDetourEnabled)
	{
		RemoveHandleBuyDetour();
		m_HandleBuyDetourEnabled = false;
	}
	if (m_CSWeaponDetourEnabled)
	{
		RemoveCSWeaponDropDetour();
		m_CSWeaponDetourEnabled = false;
	}

	g_RegNatives.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
	plsys->RemovePluginsListener(this);
	playerhelpers->UnregisterCommandTargetProcessor(this);
	forwards->ReleaseForward(g_pHandleBuyForward);
	forwards->ReleaseForward(g_pPriceForward);
	forwards->ReleaseForward(g_pTerminateRoundForward);
	forwards->ReleaseForward(g_pCSWeaponDropForward);

#if SOURCE_ENGINE == SE_CSGO
	rulesfix.OnUnload();
	ClearHashMaps();
#endif
}

void CStrike::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	if (g_pSDKTools == NULL)
	{
		smutils->LogError(myself, "SDKTools interface not found. TerminateRound native disabled.");
	}
	else if (g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		smutils->LogError(myself, "SDKTools interface is outdated. TerminateRound native disabled.");
	}
	gameevents->AddListener(&g_TimeLeftEvents, "round_start", true);
	gameevents->AddListener(&g_TimeLeftEvents, "round_end", true);
	SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(&g_TimeLeftEvents, &TimeLeftEvents::LevelInit), true);
	hooked_everything = true;

	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

#if SOURCE_ENGINE == SE_CSGO
	CreateHashMaps();
#endif
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

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

bool CStrike::ProcessCommandTarget(cmd_target_info_t *info)
{
	int max_clients;
	IPlayerInfo *pInfo;
	unsigned int team_index = 0;
	IGamePlayer *pPlayer, *pAdmin;

	if ((info->flags & COMMAND_FILTER_NO_MULTI) == COMMAND_FILTER_NO_MULTI)
	{
		return false;
	}

	if (info->admin)
	{
		if ((pAdmin = playerhelpers->GetGamePlayer(info->admin)) == NULL)
		{
			return false;
		}
		if (!pAdmin->IsInGame())
		{
			return false;
		}
	}
	else
	{
		pAdmin = NULL;
	}

	if (strcmp(info->pattern, "@ct") == 0 || strcmp(info->pattern, "@cts") == 0)
	{
		team_index = 3;
	}
	else if (strcmp(info->pattern, "@t") == 0 || strcmp(info->pattern, "@ts") == 0)
	{
		team_index = 2;
	}
	else
	{
		return false;
	}

	info->num_targets = 0;

	max_clients = playerhelpers->GetMaxClients();
	for (int i = 1; 
		 i <= max_clients && (cell_t)info->num_targets < info->max_targets; 
		 i++)
	{
		if ((pPlayer = playerhelpers->GetGamePlayer(i)) == NULL)
		{
			continue;
		}
		if (!pPlayer->IsInGame())
		{
			continue;
		}
		if ((pInfo = pPlayer->GetPlayerInfo()) == NULL)
		{
			continue;
		}
		if (pInfo->GetTeamIndex() != (int)team_index)
		{
			continue;
		}
		if (playerhelpers->FilterCommandTarget(pAdmin, pPlayer, info->flags) 
			!= COMMAND_TARGET_VALID)
		{
			continue;
		}
		info->targets[info->num_targets] = i;
		info->num_targets++;
	}

	if (info->num_targets == 0)
	{
		info->reason = COMMAND_TARGET_EMPTY_FILTER;
	}
	else
	{
		info->reason = COMMAND_TARGET_VALID;
	}

	info->target_name_style = COMMAND_TARGETNAME_RAW;
	if (team_index == 2)
	{
		UTIL_Format(info->target_name, info->target_name_maxlength, "Terrorists");
	}
	else if (team_index == 3)
	{
		UTIL_Format(info->target_name, info->target_name_maxlength, "CTs");
	}

	return true;
}

const char *CStrike::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *CStrike::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

void CStrike::OnPluginLoaded(IPlugin *plugin)
{
	if (!m_WeaponPriceDetourEnabled && g_pPriceForward->GetFunctionCount())
	{
		m_WeaponPriceDetourEnabled = CreateWeaponPriceDetour();
		if (m_WeaponPriceDetourEnabled)
			m_HandleBuyDetourEnabled = true;
	}
	if (!m_TerminateRoundDetourEnabled && g_pTerminateRoundForward->GetFunctionCount())
	{
		m_TerminateRoundDetourEnabled = CreateTerminateRoundDetour();
	}
	if (!m_HandleBuyDetourEnabled && g_pHandleBuyForward->GetFunctionCount())
	{
		m_HandleBuyDetourEnabled = CreateHandleBuyDetour();
	}
	if (!m_CSWeaponDetourEnabled && g_pCSWeaponDropForward->GetFunctionCount())
	{
		m_CSWeaponDetourEnabled = CreateCSWeaponDropDetour();
	}
}

void CStrike::OnPluginUnloaded(IPlugin *plugin)
{
	if (m_WeaponPriceDetourEnabled && !g_pPriceForward->GetFunctionCount())
	{
		RemoveWeaponPriceDetour();
		m_WeaponPriceDetourEnabled = false;
	}
	if (m_TerminateRoundDetourEnabled && !g_pTerminateRoundForward->GetFunctionCount())
	{
		RemoveTerminateRoundDetour();
		m_TerminateRoundDetourEnabled = false;
	}
	if (m_HandleBuyDetourEnabled && !g_pHandleBuyForward->GetFunctionCount())
	{
		RemoveHandleBuyDetour();
		m_HandleBuyDetourEnabled = false;
	}
	if (m_CSWeaponDetourEnabled && !g_pCSWeaponDropForward->GetFunctionCount())
	{
		RemoveCSWeaponDropDetour();
		m_CSWeaponDetourEnabled = false;;
	}
}
