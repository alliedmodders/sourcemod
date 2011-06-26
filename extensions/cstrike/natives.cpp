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
#include <server_class.h>

#define REGISTER_NATIVE_ADDR(name, code) \
	void *addr; \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		return pContext->ThrowNativeError("Failed to locate function"); \
	} \
	code; \
	g_RegNatives.Register(pWrapper);

inline CBaseEntity *GetCBaseEntity(int num, bool isplayer)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}

	if (num > 0 && num <= playerhelpers->GetMaxClients())
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(pEdict);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return NULL;
		}
	}
	else if (isplayer) 
	{
		return NULL;
	}

	IServerUnknown *pUnk;
	if ((pUnk=pEdict->GetUnknown()) == NULL)
	{
		return NULL;
	}

	return pUnk->GetBaseEntity();
}

bool UTIL_FindDataTable(SendTable *pTable, const char *name, sm_sendprop_info_t *info, unsigned int offset)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;
	SendTable *table;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);

		if ((table = prop->GetDataTable()) != NULL)
		{
			pname = table->GetName();
			if (pname && strcmp(name, pname) == 0)
			{
				info->prop = prop;
				info->actual_offset = offset + info->prop->GetOffset();
				return true;
			}

			if (UTIL_FindDataTable(table, 
				name,
				info,
				offset + prop->GetOffset())
				)
			{
				return true;
			}
		}
	}

	return false;
}

//Taken From Sourcemod Core
unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

static cell_t CS_RespawnPlayer(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("RoundRespawn", 
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0));
	}

	CBaseEntity *pEntity;
	if (!(pEntity=GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}
	pWrapper->Execute(&pEntity, NULL);

	return 1;
}

static cell_t CS_SwitchTeam(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("SwitchTeam", 
			PassInfo pass[1]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(int); \
			pass[0].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 1))
	}

	CBaseEntity *pEntity;
	if (!(pEntity=GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(int)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(int *)vptr = params[2];
	pWrapper->Execute(vstk, NULL);

	return 1;
}

static cell_t CS_DropWeapon(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("CSWeaponDrop",
			PassInfo pass[3]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type  = PassType_Basic; \
			pass[0].size  = sizeof(CBaseEntity *); \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].type  = PassType_Basic; \
			pass[1].size  = sizeof(bool); \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].type  = PassType_Basic; \
			pass[2].size  = sizeof(bool); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 3))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pWeapon;
	if (!(pWeapon = GetCBaseEntity(params[2], false)))
	{
		return pContext->ThrowNativeError("Weapon index %d is not valid", params[2]);
	}

	//Psychonic is awesome for this
	sm_sendprop_info_t *spi = new sm_sendprop_info_t;
	IServerUnknown *pUnk = (IServerUnknown *)pWeapon;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!UTIL_FindDataTable(pNet->GetServerClass()->m_pTable, "DT_WeaponCSBase", spi, 0))
		return pContext->ThrowNativeError("Entity index %d is not a weapon", params[2]);

	if (!gamehelpers->FindSendPropInfo("CBaseCombatWeapon", "m_hOwnerEntity", spi))
		return pContext->ThrowNativeError("Invalid entity index %d for weapon", params[2]);

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pWeapon + spi->actual_offset);
	if (params[1] != hndl.GetEntryIndex())
		return pContext->ThrowNativeError("Weapon %d is not owned by client %d", params[2], params[1]);

	if (params[4] == 1 && g_pCSWeaponDropDetoured)
		g_pIgnoreCSWeaponDropDetour = true;

	unsigned char vstk[sizeof(CBaseEntity *) * 2 + sizeof(bool) * 2];
	unsigned char *vptr = vstk;

	// <psychonic> first one is always false. second is true to toss, false to just drop
	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = pWeapon;
	vptr += sizeof(CBaseEntity *);
	*(bool *)vptr = false;
	vptr += sizeof(bool);
	*(bool *)vptr = (params[3]) ? true : false;

 	pWrapper->Execute(vstk, NULL);

	return 1;
}

static cell_t CS_TerminateRound(IPluginContext *pContext, const cell_t *params)
{
	if (g_pSDKTools == NULL)
	{
		smutils->LogError(myself, "SDKTools interface not found. TerminateRound native disabled.");
	}
	else if (g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		return pContext->ThrowNativeError("SDKTools interface is outdated. TerminateRound native disabled.");
	}

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("TerminateRound",
			PassInfo pass[2]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(float); \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].type = PassType_Basic; \
			pass[1].size = sizeof(int); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2))
	}

	void *gamerules = g_pSDKTools->GetGameRules();
	if (gamerules == NULL)
	{
		return pContext->ThrowNativeError("GameRules not available. TerminateRound native disabled.");
	}

	if (params[3] == 1 && g_pTerminateRoundDetoured)
		g_pIgnoreTerminateDetour = true;

	unsigned char vstk[sizeof(void *) + sizeof(float)+ sizeof(int)];
	unsigned char *vptr = vstk;

	*(void **)vptr = gamerules;
	vptr += sizeof(void *);
	*(float *)vptr = sp_ctof(params[1]);
	vptr += sizeof(float);
	*(int*)vptr = params[2];

	pWrapper->Execute(vstk, NULL);

	return 1;
}

static cell_t CS_GetTranslatedWeaponAlias(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("GetTranslatedWeaponAlias",
			PassInfo pass[1]; \
			PassInfo retpass[1]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(char *); \
			retpass[0].flags = PASSFLAG_BYVAL; \
			retpass[0].type = PassType_Basic; \
			retpass[0].size = sizeof(char *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass[0], pass, 1))
	}
	char *dest, *weapon;

	pContext->LocalToString(params[2], &dest);
	pContext->LocalToString(params[1], &weapon);

	char *ret = new char[128];

	unsigned char vstk[sizeof(char *)];
	unsigned char *vptr = vstk;

	*(char **)vptr = weapon;

	pWrapper->Execute(vstk, &ret);

	strncopy(dest, ret, params[3]);
	
	return 1;
}

sp_nativeinfo_t g_CSNatives[] = 
{
	{"CS_RespawnPlayer",			CS_RespawnPlayer}, 
	{"CS_SwitchTeam",				CS_SwitchTeam},
	{"CS_DropWeapon",				CS_DropWeapon},
	{"CS_TerminateRound",			CS_TerminateRound},
	{"CS_GetTranslatedWeaponAlias",	CS_GetTranslatedWeaponAlias},
	{NULL,							NULL}
};

