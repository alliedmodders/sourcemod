/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
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
#include "conditions.h"
#include "util.h"

#include <iplayerinfo.h>

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

template<PlayerConditionsMgr::CondVar CondVar>
static void OnPlayerCondChange(const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	g_CondMgr.OnConVarChange(CondVar, pProp, pStructBase, pData, pOut, iElement, objectID);
}

void PlayerConditionsMgr::OnConVarChange(CondVar var, const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	CBaseEntity *pPlayer = (CBaseEntity *)((intp) pStructBase - GetPropOffs(m_Shared));
	int client = gamehelpers->EntityToBCompatRef(pPlayer);

	int newConds = 0;
	int prevConds = 0;

	if (var == m_nPlayerCond)
	{
		prevConds = m_OldConds[client][_condition_bits] | m_OldConds[client][var];
		newConds = m_OldConds[client][_condition_bits] | *(int *) (pData);
	}
	else if (var == _condition_bits)
	{
		prevConds = m_OldConds[client][m_nPlayerCond] | m_OldConds[client][var];
		newConds = m_OldConds[client][m_nPlayerCond] | *(int *)(pData);
	}
	else
	{
		prevConds = m_OldConds[client][var];
		newConds = *(int *)pData;
	}

	if (prevConds != newConds)
	{
		int changedConds = newConds ^ prevConds;
		int addedConds = changedConds & newConds;
		int removedConds = changedConds & prevConds;
		m_OldConds[client][var] = newConds;

		for (int i = 0; i < 32; i++)
		{
			if (addedConds & (1 << i))
			{
				g_addCondForward->PushCell(client);
				g_addCondForward->PushCell(i);
				g_addCondForward->Execute(NULL, NULL);
			}
			else if (removedConds & (1 << i))
			{
				g_removeCondForward->PushCell(client);
				g_removeCondForward->PushCell(i);
				g_removeCondForward->Execute(NULL, NULL);
			}
		}
	}

	if (m_BackupProxyFns[var] != nullptr)
		m_BackupProxyFns[var](pProp, pStructBase, pData, pOut, iElement, objectID);
}

template <PlayerConditionsMgr::CondVar var>
bool PlayerConditionsMgr::SetupProp(const char *varname)
{
	if (!gamehelpers->FindSendPropInfo("CTFPlayer", varname, &m_CondVarProps[var]))
	{
		g_pSM->LogError(myself, "Failed to find %s prop offset", varname);
		return false;
	}

	if (var != m_Shared)
	{
		m_BackupProxyFns[var] = GetProp(var)->GetProxyFn();
		GetProp(var)->SetProxyFn(OnPlayerCondChange<var>);
	}

	return true;
}

bool PlayerConditionsMgr::Init()
{
	memset(m_BackupProxyFns, 0, sizeof(m_BackupProxyFns));

	bool bFoundProps = SetupProp<m_nPlayerCond>("m_nPlayerCond")
		&& SetupProp<_condition_bits>("_condition_bits")
		&& SetupProp<m_nPlayerCondEx>("m_nPlayerCondEx")
		&& SetupProp<m_nPlayerCondEx2>("m_nPlayerCondEx2")
		&& SetupProp<m_nPlayerCondEx3>("m_nPlayerCondEx3")
		&& SetupProp<m_Shared>("m_Shared");

	if (!bFoundProps)
		return false;

	playerhelpers->AddClientListener(this);

	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		for (size_t j = 0; j < CondVar_LastNotifyProp; ++j)
		{
			m_OldConds[i][j] = *(int *)((intp) pEntity + GetPropOffs((CondVar)j));
		}
	}

	return true;
}

void PlayerConditionsMgr::Shutdown()
{
	for (size_t i = 0; i < CondVar_LastNotifyProp; ++i)
	{
		GetProp((CondVar)i)->SetProxyFn(m_BackupProxyFns[i]);
	}

	playerhelpers->RemoveClientListener(this);
}

void PlayerConditionsMgr::OnClientPutInServer(int client)
{
	memset(&m_OldConds[client], 0, sizeof(m_OldConds[0]));
}

PlayerConditionsMgr g_CondMgr;