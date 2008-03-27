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

#include "extension.h"
#include "RegNatives.h"
#include "iplayerinfo.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */


TF2Tools g_TF2Tools;
IGameConfig *g_pGameConf = NULL;

IBinTools *g_pBinTools = NULL;

SMEXT_LINK(&g_TF2Tools);

SendProp *playerSharedOffset;

extern sp_nativeinfo_t g_TFNatives[];

bool TF2Tools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile("sm-tf2.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (error)
		{
			snprintf(error, maxlength, "Could not read sm-tf2.games.txt: %s", conf_error);
		}
		return false;
	}

	sharesys->AddNatives(myself, g_TFNatives);
	sharesys->RegisterLibrary(myself, "tf2");

	playerSharedOffset = gamehelpers->FindInSendTable("CTFPlayer", "DT_TFPlayerShared");

	playerhelpers->RegisterCommandTargetProcessor(this);

	return true;
}

bool TF2Tools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	return true;
}

void TF2Tools::SDK_OnUnload()
{
	g_RegNatives.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void TF2Tools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
}

bool TF2Tools::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool TF2Tools::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void TF2Tools::NotifyInterfaceDrop(SMInterface *pInterface)
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

bool TF2Tools::ProcessCommandTarget(cmd_target_info_t *info)
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

	if (strcmp(info->pattern, "@red") == 0 )
	{
		team_index = 2;
	}
	else if (strcmp(info->pattern, "@blue") == 0)
	{
		team_index = 3;
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
		UTIL_Format(info->target_name, info->target_name_maxlength, "Red Team");
	}
	else if (team_index == 3)
	{
		UTIL_Format(info->target_name, info->target_name_maxlength, "Blue Team");
	}

	return true;
}

