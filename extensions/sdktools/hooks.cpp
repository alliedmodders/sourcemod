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

CHookManager g_Hooks;
static bool PRCH_enabled = false;

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

	m_usercmdsFwd = forwards->CreateForward("OnPlayerRunCmd", ET_Event, 6, NULL, Param_Cell, Param_CellByRef, Param_CellByRef, Param_Array, Param_Array, Param_CellByRef);
}

void CHookManager::Shutdown()
{
	forwards->ReleaseForward(m_usercmdsFwd);
}

void CHookManager::OnClientPutInServer(int client)
{
	if (!PRCH_enabled)
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

	SH_ADD_MANUALHOOK_MEMFUNC(PlayerRunCmdHook, pEntity, this, &CHookManager::PlayerRunCmd, false);
}

void CHookManager::OnClientDisconnecting(int client)
{
	if (!PRCH_enabled)
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

	SH_REMOVE_MANUALHOOK_MEMFUNC(PlayerRunCmdHook, pEntity, this, &CHookManager::PlayerRunCmd, false);
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

	m_usercmdsFwd->PushCell(client);
	m_usercmdsFwd->PushCellByRef(&ucmd->buttons);
	m_usercmdsFwd->PushCellByRef(&impulse);
	m_usercmdsFwd->PushArray(vel, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushArray(angles, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushCellByRef(&ucmd->weaponselect);
	m_usercmdsFwd->Execute(&result);

	ucmd->impulse = impulse;
	ucmd->forwardmove = sp_ctof(vel[0]);
	ucmd->sidemove = sp_ctof(vel[1]);
	ucmd->upmove = sp_ctof(vel[2]);
	ucmd->viewangles.x = sp_ctof(angles[0]);
	ucmd->viewangles.y = sp_ctof(angles[1]);
	ucmd->viewangles.z = sp_ctof(angles[2]);


	if (result == Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}
