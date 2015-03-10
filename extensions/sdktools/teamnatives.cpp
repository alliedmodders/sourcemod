/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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
#include "vhelpers.h"
#include <sh_vector.h>

struct TeamInfo
{
	const char *ClassName;
	CBaseEntity *pEnt;
};

const char *m_iScore;

SourceHook::CVector<TeamInfo> g_Teams;

void InitTeamNatives()
{
	g_Teams.clear();
	g_Teams.resize(1);

	int edictCount = gpGlobals->maxEntities;

	for (int i=0; i<edictCount; i++)
	{
		edict_t *pEdict = PEntityOfEntIndex(i);
		if (!pEdict || pEdict->IsFree())
		{
			continue;
		}
		if (!pEdict->GetNetworkable())
		{
			continue;
		}

		ServerClass *pClass = pEdict->GetNetworkable()->GetServerClass();
		if (FindNestedDataTable(pClass->m_pTable, "DT_Team"))
		{
			SendProp *pTeamNumProp = g_pGameHelpers->FindInSendTable(pClass->GetName(), "m_iTeamNum");

			if (pTeamNumProp != NULL)
			{
				int offset = pTeamNumProp->GetOffset();
				CBaseEntity *pEnt = pEdict->GetUnknown()->GetBaseEntity();
				int TeamIndex = *(int *)((unsigned char *)pEnt + offset);

				if (TeamIndex >= (int)g_Teams.size())
				{
					g_Teams.resize(TeamIndex+1);
				}
				g_Teams[TeamIndex].ClassName = pClass->GetName();
				g_Teams[TeamIndex].pEnt = pEnt;
			}
		}
	}
}

static cell_t GetTeamCount(IPluginContext *pContext, const cell_t *params)
{
	return g_Teams.size();
}

static int g_teamname_offset = -1;

const char *tools_GetTeamName(int team)
{
	if (size_t(team) >= g_Teams.size())
		return NULL;
	if (g_teamname_offset == 0)
		return NULL;
	if (g_teamname_offset == -1)
	{
		SendProp *prop = g_pGameHelpers->FindInSendTable(g_Teams[team].ClassName, "m_szTeamname");
		if (prop == NULL)
		{
			g_teamname_offset = 0;
			return NULL;
		}
		g_teamname_offset = prop->GetOffset();
	}

	return (const char *)((unsigned char *)g_Teams[team].pEnt + g_teamname_offset);
}

static cell_t GetTeamName(IPluginContext *pContext, const cell_t *params)
{
	int teamindex = params[1];
	if (teamindex >= (int)g_Teams.size() || !g_Teams[teamindex].ClassName)
		return pContext->ThrowNativeError("Team index %d is invalid", teamindex);

	if (g_teamname_offset == 0)
		return pContext->ThrowNativeError("Team names are not available on this game.");

	const char *name = tools_GetTeamName(teamindex);
	if (name == NULL)
		return pContext->ThrowNativeError("Team names are not available on this game.");

	pContext->StringToLocalUTF8(params[2], params[3], name, NULL);

	return 1;
}

static cell_t GetTeamScore(IPluginContext *pContext, const cell_t *params)
{
	int teamindex = params[1];
	if (teamindex >= (int)g_Teams.size() || !g_Teams[teamindex].ClassName)
	{
		return pContext->ThrowNativeError("Team index %d is invalid", teamindex);
	}

	if (!m_iScore)
	{
		m_iScore = g_pGameConf->GetKeyValue("m_iScore");
		if (!m_iScore)
		{
			return pContext->ThrowNativeError("Failed to get m_iScore key");
		}
	}

	static int offset = -1;

	if (offset == -1)
	{
		SendProp *prop = g_pGameHelpers->FindInSendTable(g_Teams[teamindex].ClassName, m_iScore);
		if (!prop)
		{
			return pContext->ThrowNativeError("Failed to get m_iScore prop");
		}
		offset = prop->GetOffset();
	}


	return *(int *)((unsigned char *)g_Teams[teamindex].pEnt + offset);
}

static cell_t SetTeamScore(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pSM->IsMapRunning())
	{
		return pContext->ThrowNativeError("Cannot set team score when no map is running");
	}
	
	int teamindex = params[1];
	if (teamindex >= (int)g_Teams.size() || !g_Teams[teamindex].ClassName)
	{
		return pContext->ThrowNativeError("Team index %d is invalid", teamindex);
	}

	if (m_iScore == NULL)
	{
		m_iScore = g_pGameConf->GetKeyValue("m_iScore");
		if (m_iScore == NULL)
		{
			return pContext->ThrowNativeError("Failed to get m_iScore key");
		}
	}

	static int offset = -1;

	if (offset == -1)
	{
		SendProp *prop = g_pGameHelpers->FindInSendTable(g_Teams[teamindex].ClassName, m_iScore);
		if (!prop)
		{
			return pContext->ThrowNativeError("Failed to get m_iScore prop");
		}
		offset = prop->GetOffset();
	}

	CBaseEntity *pTeam = g_Teams[teamindex].pEnt;
	*(int *)((unsigned char *)pTeam + offset) = params[2];

	edict_t *pEdict = gameents->BaseEntityToEdict(pTeam);
	gamehelpers->SetEdictStateChanged(pEdict, offset);

	return 1;
}

static cell_t GetTeamClientCount(IPluginContext *pContext, const cell_t *params)
{
	int teamindex = params[1];
	if (teamindex >= (int)g_Teams.size() || !g_Teams[teamindex].ClassName)
	{
		return pContext->ThrowNativeError("Team index %d is invalid", teamindex);
	}

	SendProp *pProp = g_pGameHelpers->FindInSendTable(g_Teams[teamindex].ClassName, "\"player_array\"");
	ArrayLengthSendProxyFn fn = pProp->GetArrayLengthProxy();

	return fn(g_Teams[teamindex].pEnt, 0);
}

static cell_t GetTeamEntity(IPluginContext *pContext, const cell_t *params)
{
	int teamindex = params[1];
	if (teamindex >= (int)g_Teams.size() || !g_Teams[teamindex].ClassName)
	{
		return pContext->ThrowNativeError("Team index %d is invalid", teamindex);
	}

	return gamehelpers->EntityToBCompatRef(g_Teams[teamindex].pEnt);
}

sp_nativeinfo_t g_TeamNatives[] = 
{
	{"GetTeamCount",			GetTeamCount},
	{"GetTeamName",				GetTeamName},
	{"GetTeamScore",			GetTeamScore},
	{"SetTeamScore",			SetTeamScore},
	{"GetTeamClientCount",		GetTeamClientCount},
	{"GetTeamEntity",			GetTeamEntity},
	{NULL,						NULL}
};
