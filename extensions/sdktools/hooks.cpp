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

#include "hooks.h"
#include "extension.h"

#include "basehandle.h"
#include "vector.h"
#include "utlvector.h"
#include <shareddefs.h>
#include "usercmd.h"

#define FEATURECAP_PLAYERRUNCMD_11PARAMS	"SDKTools PlayerRunCmd 11Params"

CHookManager g_Hooks;
static bool PRCH_enabled = false;
static bool PRCH_used = false;

SH_DECL_MANUALHOOK2_void(PlayerRunCmdHook, 0, 0, 0, CUserCmd *, IMoveHelper *);

CHookManager::CHookManager()
{
	m_usercmdsFwd = NULL;
}

void CHookManager::Initialize()
{
	int offset;
	if (g_pGameConf->GetOffset("PlayerRunCmd", &offset))
	{
		SH_MANUALHOOK_RECONFIGURE(PlayerRunCmdHook, offset, 0, 0);
		PRCH_enabled = true;
	}
	else
	{
		g_pSM->LogError(myself, "Failed to find PlayerRunCmd offset - OnPlayerRunCmd forward disabled.");
		PRCH_enabled = false;
	}
	
	plsys->AddPluginsListener(this);
	sharesys->AddCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);

	m_usercmdsFwd = forwards->CreateForward("OnPlayerRunCmd", ET_Event, 11, NULL,
		Param_Cell,			// client
		Param_CellByRef,	// buttons
		Param_CellByRef,	// impulse
		Param_Array,		// Float:vel[3]
		Param_Array,		// Float:angles[3]
		Param_CellByRef,	// weapon
		Param_CellByRef,	// subtype
		Param_CellByRef,	// cmdnum
		Param_CellByRef,	// tickcount
		Param_CellByRef,	// seed
		Param_Array);		// mouse[2]
}

void CHookManager::Shutdown()
{
	forwards->ReleaseForward(m_usercmdsFwd);

	plsys->RemovePluginsListener(this);
	sharesys->DropCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);
}

void CHookManager::OnClientPutInServer(int client)
{
	if (!PRCH_enabled)
		return;

	if (!PRCH_used)
		return;

	edict_t *pEdict = PEntityOfEntIndex(client);
	if (!pEdict)
	{
		return;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return;
	}

	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
	if (!pEntity)
	{
		return;
	}

	SH_ADD_MANUALHOOK(PlayerRunCmdHook, pEntity, SH_MEMBER(this, &CHookManager::PlayerRunCmd), false);
}

void CHookManager::OnClientDisconnecting(int client)
{
	if (!PRCH_enabled)
		return;

	if (!PRCH_used)
		return;

	edict_t *pEdict = PEntityOfEntIndex(client);
	if (!pEdict)
	{
		return;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return;
	}

	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
	if (!pEntity)
	{
		return;
	}

	SH_REMOVE_MANUALHOOK(PlayerRunCmdHook, pEntity, SH_MEMBER(this, &CHookManager::PlayerRunCmd), false);
}

void CHookManager::PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (m_usercmdsFwd->GetFunctionCount() == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	if (!pEntity)
	{
		RETURN_META(MRES_IGNORED);
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);

	if (!pEdict)
	{
		RETURN_META(MRES_IGNORED);
	}

	int client = IndexOfEdict(pEdict);


	cell_t result = 0;
	/* Impulse is a byte so we copy it back manually */
	cell_t impulse = ucmd->impulse;
	cell_t vel[3] = {sp_ftoc(ucmd->forwardmove), sp_ftoc(ucmd->sidemove), sp_ftoc(ucmd->upmove)};
	cell_t angles[3] = {sp_ftoc(ucmd->viewangles.x), sp_ftoc(ucmd->viewangles.y), sp_ftoc(ucmd->viewangles.z)};
	cell_t mouse[2] = {ucmd->mousedx, ucmd->mousedy};

	m_usercmdsFwd->PushCell(client);
	m_usercmdsFwd->PushCellByRef(&ucmd->buttons);
	m_usercmdsFwd->PushCellByRef(&impulse);
	m_usercmdsFwd->PushArray(vel, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushArray(angles, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushCellByRef(&ucmd->weaponselect);
	m_usercmdsFwd->PushCellByRef(&ucmd->weaponsubtype);
	m_usercmdsFwd->PushCellByRef(&ucmd->command_number);
	m_usercmdsFwd->PushCellByRef(&ucmd->tick_count);
	m_usercmdsFwd->PushCellByRef(&ucmd->random_seed);
	m_usercmdsFwd->PushArray(mouse, 2, SM_PARAM_COPYBACK);
	m_usercmdsFwd->Execute(&result);

	ucmd->impulse = impulse;
	ucmd->forwardmove = sp_ctof(vel[0]);
	ucmd->sidemove = sp_ctof(vel[1]);
	ucmd->upmove = sp_ctof(vel[2]);
	ucmd->viewangles.x = sp_ctof(angles[0]);
	ucmd->viewangles.y = sp_ctof(angles[1]);
	ucmd->viewangles.z = sp_ctof(angles[2]);
	ucmd->mousedx = mouse[0];
	ucmd->mousedy = mouse[1];


	if (result == Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void CHookManager::OnPluginLoaded(IPlugin *plugin)
{
	if (!PRCH_enabled)
		return;

	if (PRCH_used)
		return;

	if (!m_usercmdsFwd->GetFunctionCount())
		return;

	PRCH_used = true;

	int MaxClients = playerhelpers->GetMaxClients();
	for (int i = 1; i <= MaxClients; i++)
	{
		if (playerhelpers->GetGamePlayer(i)->IsInGame())
		{
			OnClientPutInServer(i);
		}
	}
}

void CHookManager::OnPluginUnloaded(IPlugin *plugin)
{
	if (!PRCH_enabled)
		return;

	if (!PRCH_used)
		return;

	if (m_usercmdsFwd->GetFunctionCount())
		return;

	int MaxClients = playerhelpers->GetMaxClients();
	for (int i = 1; i <= MaxClients; i++)
	{
		if (playerhelpers->GetGamePlayer(i)->IsInGame())
		{
			OnClientDisconnecting(i);
		}
	}

	PRCH_used = false;
}

FeatureStatus CHookManager::GetFeatureStatus(FeatureType type, const char *name)
{
	return FeatureStatus_Available;
}
