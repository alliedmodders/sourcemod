/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
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
#include "util.h"
#include "time.h"
#include "RegNatives.h"

#include <ISDKTools.h>
#include <sm_argbuffer.h>

// native TF2_MakeBleed(client, attacker, Float:duration)
cell_t TF2_MakeBleed(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::MakeBleed(CTFPlayer*, CTFWeaponBase*, float, int=4, bool=false)
	if(!pWrapper)
	{
		REGISTER_NATIVE_ADDR("MakeBleed",
			PassInfo pass[6]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(CBaseEntity *); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(CBaseEntity *); \
			pass[1].type = PassType_Basic; \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].size = sizeof(float); \
			pass[2].type = PassType_Float; \
			pass[3].flags = PASSFLAG_BYVAL; \
			pass[3].size = sizeof(int); \
			pass[3].type = PassType_Basic; \
			pass[4].flags = PASSFLAG_BYVAL; \
			pass[4].size = sizeof(bool); \
			pass[4].type = PassType_Basic; \
			pass[5].flags = PASSFLAG_BYVAL; \
			pass[5].size = sizeof(int); \
			pass[5].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 6))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pAttacker;
	if (!(pAttacker = UTIL_GetCBaseEntity(params[2], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[2]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*, CBaseEntity*, CBaseEntity*, 
											float,
											int, // Damage amount
											bool, // Permanent
											int>  // Custom Damage type (bleeding)
											vstk(obj, pAttacker, NULL, sp_ctof(params[3]), 4, false, 34);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

// native TF2_Burn(client, target, duration)
cell_t TF2_Burn(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::Burn(CTFPlayer*, CTFWeaponBase*, float=-1.0)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("Burn", 
			PassInfo pass[3]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(CBaseEntity *); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(CBaseEntity *); \
			pass[1].type = PassType_Basic; \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].size = sizeof(float); \
			pass[2].type = PassType_Float; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 3))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pTarget;
	if (!(pTarget = UTIL_GetCBaseEntity(params[2], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[2]);
	}

	float fDuration = 10.0;
	// Compatibility fix for the newly-added duration
	if (params[0] >= 3)
	{
		fDuration = sp_ctof(params[3]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*, CBaseEntity*, CBaseEntity*, 
										float> //duration
										vstk(obj, pTarget, nullptr, fDuration);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

// native TF2_Invuln(client, bool:enabled)
cell_t TF2_Invuln(IPluginContext *pContext, const cell_t *params)
{
	return pContext->ThrowNativeError("TF2_SetPlayerInvuln is no longer available");
}

cell_t TF2_Disguise(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayerShared::Disguise(int, int, CTFPlayer *, bool=true)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("Disguise", 
			PassInfo pass[4]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(int); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(int); \
			pass[1].type = PassType_Basic; \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].size = sizeof(CBaseEntity *); \
			pass[2].type = PassType_Basic; \
			pass[3].flags = PASSFLAG_BYVAL; \
			pass[3].size = sizeof(bool); \
			pass[3].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 4))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	
	CBaseEntity *pTarget = NULL;
	// Compatibility fix for the newly-added target parameter
	if (params[0] >= 4 && params[4] > 0 && !(pTarget = UTIL_GetCBaseEntity(params[4], true)))
	{
		return pContext->ThrowNativeError("Target client index %d is not valid", params[4]);
	}

	ArgBuffer<void*, int, int, CBaseEntity*, bool> vstk(obj, params[2], params[3], pTarget, true);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_RemoveDisguise(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayerShared::RemoveDisguise()
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("RemoveDisguise", 
				pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*> vstk(obj);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_AddCondition(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::AddCond(int, float, CBaseEntity *)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("AddCondition", 
			PassInfo pass[3]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(int); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(float); \
			pass[1].type = PassType_Float; \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].size = sizeof(CBaseEntity *); \
			pass[2].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 3))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pInflictor = NULL;
	// Compatibility fix for the new inflictor parameter; TF2 seems to only use player entities in it
	if (params[0] >= 4 && params[4] > 0 && !(pInflictor = UTIL_GetCBaseEntity(params[4], true)))
	{
		return pContext->ThrowNativeError("Inflictor index %d is not valid", params[4]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*, int, float, CBaseEntity*> vstk(obj, params[2], sp_ctof(params[3]), pInflictor);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_RemoveCondition(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::RemoveCond(int, bool)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("RemoveCondition", 
			PassInfo pass[2]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(int); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(bool); \
			pass[1].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*, int, bool> vstk(obj, params[2], true);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_StunPlayer(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::StunPlayer(float, float, int, CTFPlayer *)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("StunPlayer", 
			PassInfo pass[4]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(float); \
			pass[0].type = PassType_Float; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(float); \
			pass[1].type = PassType_Float; \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].size = sizeof(int); \
			pass[2].type = PassType_Basic; \
			pass[3].flags = PASSFLAG_BYVAL; \
			pass[3].size = sizeof(CBaseEntity *); \
			pass[3].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 4))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	bool bByPlayer = (params[5] != 0);
	CBaseEntity *pAttacker = NULL;
	if (bByPlayer && !(pAttacker = UTIL_GetCBaseEntity(params[5], true)))
	{
		return pContext->ThrowNativeError("Attacker index %d is not valid", params[5]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);
	ArgBuffer<void*, float, float, int, CBaseEntity*> vstk(obj, sp_ctof(params[2]), sp_ctof(params[3]), params[4], pAttacker);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_SetPowerplayEnabled(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayer::SetPowerPlayEnabled(bool)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("SetPowerplayEnabled", 
			PassInfo pass[1]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(bool); \
			pass[0].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 1))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	ArgBuffer<void*, bool> vstk(pEntity, params[2] != 0);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_Respawn(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayer::ForceRespawn()

	if (!pWrapper)
	{
		int offset;

		if (!g_pGameConf->GetOffset("ForceRespawn", &offset))
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}

		pWrapper = g_pBinTools->CreateVCall(offset,
											0,
											0,
											NULL,
											NULL,
											0);

		g_RegNatives.Register(pWrapper);
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	ArgBuffer<void*> vstk(pEntity);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_Regenerate(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayer::Regenerate( bool=true )

	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("Regenerate", 
			PassInfo pass[1]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(bool); \
			pass[0].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 1))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}
	
	ArgBuffer<void*, bool> vstk(pEntity, true);
	
	pWrapper->Execute(vstk, nullptr);
	return 1;
}

cell_t TF2_GetResourceEntity(IPluginContext *pContext, const cell_t *params)
{
	return g_resourceEntity;
}

cell_t TF2_GetClass(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	return (cell_t)ClassnameToType(str);
}

// native TF2_IsPlayerInDuel(client)
cell_t TF2_IsPlayerInDuel(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// DuelMiniGame_IsPlayerInDuel(CTFPlayer *)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("IsPlayerInDuel", 
			PassInfo pass[1]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(CBaseEntity *); \
			pass[0].type = PassType_Basic; \
			PassInfo ret; \
			ret.flags = PASSFLAG_BYVAL; \
			ret.size = sizeof(bool); \
			ret.type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &ret, pass, 1))
	}

	CBaseEntity *pPlayer;
	if (!(pPlayer = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	ArgBuffer<CBaseEntity*> vstk(pPlayer);
	
	bool retValue;
	pWrapper->Execute(vstk, &retValue);

	return (retValue) ? 1 : 0;
}

// native bool:TF2_IsHolidayActive(TFHoliday:holiday);
cell_t TF2_IsHolidayActive(IPluginContext *pContext, const cell_t *params)
{
	void *pGameRules;
	if (!g_pSDKTools || !(pGameRules = g_pSDKTools->GetGameRules()))
	{
		return pContext->ThrowNativeError("Failed to find GameRules");
	}

	static ICallWrapper *pWrapper = NULL;

	// CTFGameRules::IsHolidayActive(int)
	if (!pWrapper)
	{
		int offset;
		if (!g_pGameConf->GetOffset("IsHolidayActive", &offset))
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}

		PassInfo pass[1];
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(int);
		pass[0].type = PassType_Basic;
		PassInfo ret;
		ret.flags = PASSFLAG_BYVAL;
		ret.size = sizeof(bool);
		ret.type = PassType_Basic;

		pWrapper = g_pBinTools->CreateVCall(offset, 0, 0, &ret, pass, 1);

		g_RegNatives.Register(pWrapper);
	}

	ArgBuffer<void*, int> vstk(pGameRules, params[1]);

	bool retValue;
	pWrapper->Execute(vstk, &retValue);

	return (retValue) ? 1 : 0;
}

cell_t TF2_RemoveWearable(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CBasePlayer::RemoveWearable(CEconWearable *)

	if (!pWrapper)
	{
		int offset;

		if (!g_pGameConf->GetOffset("RemoveWearable", &offset))
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}

		PassInfo pass[1];
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(CBaseEntity *);
		pass[0].type = PassType_Basic;

		pWrapper = g_pBinTools->CreateVCall(offset, 0, 0, NULL, pass, 1);

		g_RegNatives.Register(pWrapper);
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pWearable;
	if (!(pWearable = UTIL_GetCBaseEntity(params[2], false)))
	{
		return pContext->ThrowNativeError("Wearable index %d is not valid", params[2]);
	}

	ArgBuffer<void*, CBaseEntity*> vstk(pEntity, pWearable);

	pWrapper->Execute(vstk, nullptr);
	return 1;
}

sp_nativeinfo_t g_TFNatives[] = 
{
	{"TF2_IgnitePlayer",			TF2_Burn},
	{"TF2_SetPlayerInvuln",			TF2_Invuln},
	{"TF2_RespawnPlayer",			TF2_Respawn},
	{"TF2_DisguisePlayer",			TF2_Disguise},
	{"TF2_RemovePlayerDisguise",	TF2_RemoveDisguise},
	{"TF2_GetResourceEntity",		TF2_GetResourceEntity},
	{"TF2_GetClass",				TF2_GetClass},
	{"TF2_RegeneratePlayer",		TF2_Regenerate},
	{"TF2_AddCondition",			TF2_AddCondition},
	{"TF2_RemoveCondition",			TF2_RemoveCondition},
	{"TF2_SetPlayerPowerPlay",		TF2_SetPowerplayEnabled},
	{"TF2_StunPlayer",				TF2_StunPlayer},
	{"TF2_MakeBleed",				TF2_MakeBleed},
	{"TF2_IsPlayerInDuel",				TF2_IsPlayerInDuel},
	{"TF2_IsHolidayActive",				TF2_IsHolidayActive},
	{"TF2_RemoveWearable",			TF2_RemoveWearable},
	{NULL,							NULL}
};
