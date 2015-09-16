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
#include "forwards.h"
#include "util_cstrike.h"
#include <server_class.h>
#include <iplayerinfo.h>

int g_iPriceOffset = -1;

#define REGISTER_NATIVE_ADDR(name, code) \
	void *addr; \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		return pContext->ThrowNativeError("Failed to locate function"); \
	} \
	code; \
	g_RegNatives.Register(pWrapper);

#define GET_MEMSIG(name) \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		return pContext->ThrowNativeError("Failed to locate function"); \
	}

inline CBaseEntity *GetCBaseEntity(int num, bool isplayer)
{
	edict_t *pEdict = gamehelpers->EdictOfIndex(num);
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
#if SOURCE_ENGINE != SE_CSGO || !defined(WIN32)
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
#else
	if (g_pSDKTools == NULL)
	{
		return pContext->ThrowNativeError("SDKTools interface not found. SwitchTeam native disabled.");
	}

	static void *addr = NULL;
	if(!addr)
	{
		GET_MEMSIG("SwitchTeam");
	}

	void *gamerules = g_pSDKTools->GetGameRules();
	if (gamerules == NULL)
	{
		return pContext->ThrowNativeError("GameRules not available. SwitchTeam native disabled.");
	}

	CBaseEntity *pEntity;
	if (!(pEntity=GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	int team = params[2];

	__asm
	{
		push team
		mov ecx, pEntity
		mov ebx, gamerules
 
		call addr
	}
#endif

	return 1;
}

static cell_t CS_DropWeapon(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
#if SOURCE_ENGINE == SE_CSGO
		REGISTER_NATIVE_ADDR("CSWeaponDrop",
			PassInfo pass[3]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(CBaseEntity *); \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].type = PassType_Basic; \
			pass[1].size = sizeof(Vector); \
			pass[2].flags = PASSFLAG_BYVAL; \
			pass[2].type = PassType_Basic; \
			pass[2].size = sizeof(bool); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 3))
#else
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
#endif
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
	sm_sendprop_info_t spi;
	IServerUnknown *pUnk = (IServerUnknown *)pWeapon;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!UTIL_FindDataTable(pNet->GetServerClass()->m_pTable, "DT_WeaponCSBase", &spi, 0))
		return pContext->ThrowNativeError("Entity index %d is not a weapon", params[2]);

	if (!gamehelpers->FindSendPropInfo("CBaseCombatWeapon", "m_hOwnerEntity", &spi))
		return pContext->ThrowNativeError("Invalid entity index %d for weapon", params[2]);

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pWeapon + spi.actual_offset);
	if (params[1] != hndl.GetEntryIndex() || hndl != ((IServerEntity *)pEntity)->GetRefEHandle())
		return pContext->ThrowNativeError("Weapon %d is not owned by client %d", params[2], params[1]);

	if (params[4] == 1 && g_pCSWeaponDropDetoured)
		g_pIgnoreCSWeaponDropDetour = true;

#if SOURCE_ENGINE == SE_CSGO
	unsigned char vstk[sizeof(CBaseEntity *) * 2 + sizeof(bool) + sizeof(Vector)];
#else
	unsigned char vstk[sizeof(CBaseEntity *) * 2 + sizeof(bool) * 2];
#endif
	unsigned char *vptr = vstk;

	// <psychonic> first one is always false. second is true to toss, false to just drop
	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = pWeapon;
	vptr += sizeof(CBaseEntity *);
#if SOURCE_ENGINE == SE_CSGO
	*(Vector *)vptr = vec3_origin;
	vptr += sizeof(Vector);
	*(bool *)vptr = false;
#else
	*(bool *)vptr = false;
	vptr += sizeof(bool);
	*(bool *)vptr = (params[3]) ? true : false;
#endif

 	pWrapper->Execute(vstk, NULL);

	return 1;
}

static cell_t CS_TerminateRound(IPluginContext *pContext, const cell_t *params)
{
	if (g_pSDKTools == NULL)
	{
		return pContext->ThrowNativeError("SDKTools interface not found. TerminateRound native disabled.");
	}
	else if (g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		return pContext->ThrowNativeError("SDKTools interface is outdated. TerminateRound native disabled.");
	}

	void *gamerules = g_pSDKTools->GetGameRules();
	if (gamerules == NULL)
	{
		return pContext->ThrowNativeError("GameRules not available. TerminateRound native disabled.");
	}

	int reason = params[2];
	
#if SOURCE_ENGINE == SE_CSGO
	reason++;
#endif
	
#if SOURCE_ENGINE != SE_CSGO || !defined(WIN32)
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

	if (params[3] == 1 && g_pTerminateRoundDetoured)
		g_pIgnoreTerminateDetour = true;

	unsigned char vstk[sizeof(void *) + sizeof(float)+ sizeof(int)];
	unsigned char *vptr = vstk;

	*(void **)vptr = gamerules;
	vptr += sizeof(void *);
	*(float *)vptr = sp_ctof(params[1]);
	vptr += sizeof(float);
	*(int*)vptr = reason;

	pWrapper->Execute(vstk, NULL);
#else
	static void *addr = NULL;

	if(!addr)
	{
		GET_MEMSIG("TerminateRound");
	}

	if (params[3] == 1 && g_pTerminateRoundDetoured)
		g_pIgnoreTerminateDetour = true;

	float delay = sp_ctof(params[1]);
	
	__asm
	{
		push reason
		movss xmm1, delay
		mov ecx, gamerules
		call addr
	}

#endif
	return 1;
}

static cell_t CS_WeaponIDToAlias(IPluginContext *pContext, const cell_t *params)
{
	if (!IsValidWeaponID(params[1]))
		return pContext->ThrowNativeError("Invalid WeaponID passed for this game");

	char *dest;

	pContext->LocalToString(params[2], &dest);

	const char *ret = WeaponIDToAlias(params[1]);
	
	if (ret == NULL)
	{
		return 0;
	}
	
	return strncopy(dest, ret, params[3]);
}

static cell_t CS_GetTranslatedWeaponAlias(IPluginContext *pContext, const cell_t *params)
{
	char *weapon;
	char *dest;

	pContext->LocalToString(params[2], &dest);
	pContext->LocalToString(params[1], &weapon);

	const char *ret = GetTranslatedWeaponAlias(weapon);
	strncopy(dest, ret, params[3]);
	
	return 1;
}

static cell_t CS_GetWeaponPrice(IPluginContext *pContext, const cell_t *params)
{

	if (!IsValidWeaponID(params[2]))
		return pContext->ThrowNativeError("Invalid WeaponID passed for this game");

	int id = GetRealWeaponID(params[2]);

	//Hard code return values for weapons that dont call GetWeaponPrice and always use default value.
#if SOURCE_ENGINE == SE_CSGO

	if (id == WEAPON_C4 || id == WEAPON_KNIFE || id == WEAPON_KNIFE_GG)
		return 0;
#else
 	if (id == WEAPON_C4 || id == WEAPON_KNIFE || id == WEAPON_SHIELD)
		return 0;
	else if (id == WEAPON_KEVLAR)
		return 650;
	else if (id == WEAPON_ASSAULTSUIT)
		return 1000;
#endif
	else if (id == WEAPON_NIGHTVISION)
		return 1250;
#if SOURCE_ENGINE == SE_CSGO
	else if (id == WEAPON_DEFUSER)
		return 400;
#endif

	void *info = GetWeaponInfo(id);
		
	if (!info)
	{
		return pContext->ThrowNativeError("Failed to get weaponinfo");
	}

	CBaseEntity *pEntity;
	if (!(pEntity = GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}
#if SOURCE_ENGINE == SE_CSGO
	static ICallWrapper *pWrapper = NULL;

#if defined(WIN32)
	if(!pWrapper)
	{
		void *pGetWeaponPrice = GetWeaponPriceFunction();
		if(!pGetWeaponPrice)
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}

		PassInfo pass[2];
		PassInfo ret;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].type = PassType_Basic;
		pass[0].size = sizeof(CEconItemView *);
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].type = PassType_Basic;
		pass[1].size = sizeof(int);
		ret.flags = PASSFLAG_BYVAL;
		ret.type = PassType_Basic;
		ret.size = sizeof(int);
		pWrapper = g_pBinTools->CreateCall(pGetWeaponPrice, CallConv_ThisCall, &ret, pass, 2);
	}
#else
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("GetWeaponPrice",
		PassInfo pass[3]; \
		PassInfo ret; \
		pass[0].flags = PASSFLAG_BYVAL; \
		pass[0].type = PassType_Basic; \
		pass[0].size = sizeof(CEconItemView *); \
		pass[1].flags = PASSFLAG_BYVAL; \
		pass[1].type = PassType_Basic; \
		pass[1].size = sizeof(int); \
		pass[2].flags = PASSFLAG_BYVAL; \
		pass[2].type = PassType_Float; \
		pass[2].size = sizeof(float); \
		ret.flags = PASSFLAG_BYVAL; \
		ret.type = PassType_Basic; \
		ret.size = sizeof(int); \
		pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, &ret, pass, 3))
	}
#endif
	// Get a CEconItemView for the m4
	// Found in CCSPlayer::HandleCommand_Buy_Internal
	// Linux a1 - CCSPlayer *pEntity, v5 - Player Team, a3 - ItemLoadoutSlot -1 use default loadoutslot:
	// v4 = *(int (__cdecl **)(_DWORD, _DWORD, _DWORD))(*(_DWORD *)(a1 + 9492) + 36); // offset 9
	// v6 = v4(a1 + 9492, v5, a3);
	// Windows v5 - CCSPlayer *pEntity a4 -  ItemLoadoutSlot -1 use default loadoutslot:
	// v8 = (*(int (__stdcall **)(_DWORD, int))(*(_DWORD *)(v5 + 9472) + 32))(*(_DWORD *)(v5 + 760), a4); // offset 8
	// The function is CCSPlayerInventory::GetItemInLoadout(int, int)
	// We can pass NULL view to the GetAttribute to use default loadoutslot.
	// We only really care about m4a1/m4a4 as price differs between them
	// thisPtrOffset = 9472/9492

	static ICallWrapper *pGetView = NULL;
	static int thisPtrOffset = -1;
	CEconItemView *view = NULL;

	if(!pGetView)
	{
		int offset = -1;
		int byteOffset = -1;
		void *pHandleCommandBuy = NULL;
		if (!g_pGameConf->GetOffset("GetItemInLoadout", &offset) || offset == -1)
		{
			smutils->LogError(myself, "Failed to get GetItemInLoadout offset. Reverting to NULL ItemView");
		}
		else if (!g_pGameConf->GetOffset("CCSPlayerInventoryOffset", &byteOffset) || byteOffset == -1)
		{
			smutils->LogError(myself, "Failed to get CCSPlayerInventoryOffset offset. Reverting to NULL ItemView");
		}
		else if (!g_pGameConf->GetMemSig("HandleCommand_Buy_Internal", &pHandleCommandBuy) || !pHandleCommandBuy)
		{
			smutils->LogError(myself, "Failed to get HandleCommand_Buy_Internal function. Reverting to NULL ItemView");
		}
		else
		{
			thisPtrOffset = *(int *)((intptr_t)pHandleCommandBuy + byteOffset);

			PassInfo pass[2];
			PassInfo ret;
			pass[0].flags = PASSFLAG_BYVAL;
			pass[0].type  = PassType_Basic;
			pass[0].size  = sizeof(int);
			pass[1].flags = PASSFLAG_BYVAL;
			pass[1].type  = PassType_Basic;
			pass[1].size  = sizeof(int);

			ret.flags = PASSFLAG_BYVAL;
			ret.type = PassType_Basic;
			ret.size = sizeof(void *);

			pGetView = g_pBinTools->CreateVCall(offset, 0, 0, &ret, pass, 2);
			g_RegNatives.Register(pGetView);
		}
	}

	IPlayerInfo *playerinfo = playerhelpers->GetGamePlayer(params[1])->GetPlayerInfo();
	if(pGetView && thisPtrOffset != -1 && playerinfo)
	{
		//If the gun isnt an M4 we ignore this as M4 is the only one that differs in price based on Loadout item.
		int iLoadoutSlot = -1;
		if(id == WEAPON_M4)
		{
			iLoadoutSlot = 15;
		}

		unsigned char vstk_view[sizeof(void *) + sizeof(int) * 2];
		unsigned char *vptr_view = vstk_view;

		*(void **)vptr_view = (void *)((intptr_t)pEntity + thisPtrOffset);
		vptr_view += sizeof(void *);
		*(int *)vptr_view = playerinfo->GetTeamIndex();
		vptr_view += sizeof(int);
		*(int *)vptr_view = iLoadoutSlot;

		pGetView->Execute(vstk_view, &view);
	}

#if defined(WIN32)
	unsigned char vstk[sizeof(void *) * 2 + sizeof(int)];
#else
	unsigned char vstk[sizeof(void *) * 2 + sizeof(int) + sizeof(float)];
#endif
	unsigned char *vptr = vstk;

	*(void **)vptr = info;
	vptr += sizeof(void *);
	*(CEconItemView **)vptr = view;
	vptr += sizeof(CEconItemView *);
	*(int *)vptr = 0;
#if !defined(WIN32)
	vptr += sizeof(int);
	*(float *)vptr = 1.0;
#endif

	int price = 0;
 	pWrapper->Execute(vstk, &price);
#else
	if (g_iPriceOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponPrice", &g_iPriceOffset))
		{
			g_iPriceOffset = -1;
			return pContext->ThrowNativeError("Failed to get WeaponPrice offset");
		}
	}

	int price = *(int *)((intptr_t)info + g_iPriceOffset);
#endif

	if (params[3] || weaponNameOffset == -1)
		return price;

	const char *weapon_name = (const char *)((intptr_t)info + weaponNameOffset);

	return CallPriceForward(params[1], weapon_name, price);
}

static cell_t CS_GetClientClanTag(IPluginContext *pContext, const cell_t *params)
{
	static void *addr;
	if (!addr)
	{
		if (!g_pGameConf->GetMemSig("SetClanTag", &addr) || !addr)
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}
	}

	CBaseEntity *pEntity;
	if (!(pEntity = GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	static int tagOffsetOffset = -1;
	static int tagOffset;

	if (tagOffsetOffset == -1)
	{
		if (!g_pGameConf->GetOffset("ClanTagOffset", &tagOffsetOffset))
		{
			tagOffsetOffset = -1;
			return pContext->ThrowNativeError("Unable to find ClanTagOffset gamedata");
		}

		tagOffset = *(int *)((intptr_t)addr + tagOffsetOffset);
	}

	size_t len;

	const char *src = (char *)((intptr_t)pEntity + tagOffset);
	pContext->StringToLocalUTF8(params[2], params[3], src, &len);

	return len;
}

static cell_t CS_SetClientClanTag(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("SetClanTag",
			PassInfo pass[2]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type  = PassType_Basic; \
			pass[0].size  = sizeof(CBaseEntity *); \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].type  = PassType_Basic; \
			pass[1].size  = sizeof(char *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	char *szNewTag;
	pContext->LocalToString(params[2], &szNewTag);

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(char *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(char **)vptr = szNewTag;

	pWrapper->Execute(vstk, NULL);

	return 1;
}

static cell_t CS_AliasToWeaponID(IPluginContext *pContext, const cell_t *params)
{
	char *weapon;

	pContext->LocalToString(params[1], &weapon);

#if SOURCE_ENGINE == SE_CSGO
	if (strstr(weapon, "usp_silencer") != NULL)
	{
		return SMCSWeapon_HKP2000;
	}
	else if(strstr(weapon, "cz75a") != NULL)
	{
		return SMCSWeapon_P250;
	}
#endif

	int id = GetFakeWeaponID(AliasToWeaponID(weapon));

	if (!IsValidWeaponID(id))
		return SMCSWeapon_NONE;

	return id;
}

static cell_t CS_SetTeamScore(IPluginContext *pContext, const cell_t *params)
{
	if (g_pSDKTools == NULL)
	{
		smutils->LogError(myself, "SDKTools interface not found. CS_SetTeamScore native disabled.");
	}
	else if (g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		return pContext->ThrowNativeError("SDKTools interface is outdated. CS_SetTeamScore native disabled.");
	}

	static void *addr;
	if (!addr)
	{
		if (!g_pGameConf->GetMemSig("CheckWinLimit", &addr) || !addr)
		{
			return pContext->ThrowNativeError("Failed to locate CheckWinLimit function");
		}
	}

	static int ctTeamOffsetOffset = -1;
	static int ctTeamOffset;

	static int tTeamOffsetOffset = -1;
	static int tTeamOffset;

	if (ctTeamOffsetOffset == -1)
	{
		if (!g_pGameConf->GetOffset("CTTeamScoreOffset", &ctTeamOffsetOffset))
		{
			ctTeamOffsetOffset = -1;
			return pContext->ThrowNativeError("Unable to find CTTeamOffset gamedata");
		}

		ctTeamOffset = *(int *)((intptr_t)addr + ctTeamOffsetOffset);
	}

	if (tTeamOffsetOffset == -1)
	{
		if (!g_pGameConf->GetOffset("TTeamScoreOffset", &tTeamOffsetOffset))
		{
			tTeamOffsetOffset = -1;
			return pContext->ThrowNativeError("Unable to find CTTeamOffset gamedata");
		}

		tTeamOffset = *(int *)((intptr_t)addr + tTeamOffsetOffset);
	}

	void *gamerules = g_pSDKTools->GetGameRules();
	if (gamerules == NULL)
	{
		return pContext->ThrowNativeError("GameRules not available. CS_SetTeamScore native disabled.");
	}

	if (params[1] == 3)
	{
		*(int16_t *)((intptr_t)gamerules+ctTeamOffset) = params[2];
		return 1;
	}
	else if (params[1] == 2)
	{
		*(int16_t *)((intptr_t)gamerules+tTeamOffset) = params[2];
		return 1;
	}
	return pContext->ThrowNativeError("Invalid team index passed (%i).", params[1]);
}

static cell_t CS_GetTeamScore(IPluginContext *pContext, const cell_t *params)
{
	if (g_pSDKTools == NULL)
	{
		smutils->LogError(myself, "SDKTools interface not found. CS_GetTeamScore native disabled.");
	}
	else if (g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		return pContext->ThrowNativeError("SDKTools interface is outdated. CS_GetTeamScore native disabled.");
	}

	static void *addr;
	if (!addr)
	{
		if (!g_pGameConf->GetMemSig("CheckWinLimit", &addr) || !addr)
		{
			return pContext->ThrowNativeError("Failed to locate CheckWinLimit function");
		}
	}

	static int ctTeamOffsetOffset = -1;
	static int ctTeamOffset;

	static int tTeamOffsetOffset = -1;
	static int tTeamOffset;

	if (ctTeamOffsetOffset == -1)
	{
		if (!g_pGameConf->GetOffset("CTTeamScoreOffset", &ctTeamOffsetOffset))
		{
			ctTeamOffsetOffset = -1;
			return pContext->ThrowNativeError("Unable to find CTTeamOffset gamedata");
		}

		ctTeamOffset = *(int *)((intptr_t)addr + ctTeamOffsetOffset);
	}

	if (tTeamOffsetOffset == -1)
	{
		if (!g_pGameConf->GetOffset("TTeamScoreOffset", &tTeamOffsetOffset))
		{
			tTeamOffsetOffset = -1;
			return pContext->ThrowNativeError("Unable to find CTTeamOffset gamedata");
		}

		tTeamOffset = *(int *)((intptr_t)addr + tTeamOffsetOffset);
	}

	void *gamerules = g_pSDKTools->GetGameRules();
	if (gamerules == NULL)
	{
		return pContext->ThrowNativeError("GameRules not available. CS_GetTeamScore native disabled.");
	}

	if (params[1] == 3)
	{
		return *(int16_t *)((intptr_t)gamerules+ctTeamOffset);
	}
	else if (params[1] == 2)
	{
		return *(int16_t *)((intptr_t)gamerules+tTeamOffset);
	}
	return pContext->ThrowNativeError("Invalid team index passed (%i).", params[1]);
}

static cell_t CS_IsValidWeaponID(IPluginContext *pContext, const cell_t *params)
{
	return IsValidWeaponID(params[1]) ? 1 : 0;
}

template <typename T>
static inline T *GetPlayerVarAddressOrError(const char *pszGamedataName, IPluginContext *pContext, CBaseEntity *pPlayerEntity)
{
	char szBaseName[128];
	g_pSM->Format(szBaseName, sizeof(szBaseName), "%sBase", pszGamedataName);

	const char *pszBaseVar = g_pGameConf->GetKeyValue(szBaseName);
	if (pszBaseVar == NULL)
	{
		pContext->ThrowNativeError("Failed to locate %s key in gamedata", szBaseName);
		return NULL;
	}

	int interimOffset = 0;
	sm_sendprop_info_t info;
	if (gamehelpers->FindSendPropInfo("CCSPlayer", pszBaseVar, &info))
	{
		interimOffset = info.actual_offset;
	}
	else
	{
		datamap_t *pMap = gamehelpers->GetDataMap(pPlayerEntity);
		typedescription_t *td = gamehelpers->FindInDataMap(pMap, pszBaseVar);
		if (td)
		{
#if SOURCE_ENGINE >= SE_LEFT4DEAD
			interimOffset = td->fieldOffset;
#else
			interimOffset = td->fieldOffset[TD_OFFSET_NORMAL];
#endif
		}
	}

	if (interimOffset == 0)
	{
		pContext->ThrowNativeError("Failed to find property \"%s\" on player.", pszBaseVar);
		return NULL;
	}
		

	int tempOffset;
	if (!g_pGameConf->GetOffset(pszGamedataName, &tempOffset))
	{
		pContext->ThrowNativeError("Failed to locate %s offset in gamedata", pszGamedataName);
		return NULL;
	}

	return (T *)((intptr_t)pPlayerEntity + interimOffset + tempOffset);
}

template <typename T>
static inline cell_t GetPlayerVar(IPluginContext *pContext, const cell_t *params, const char *varName)
{
	CBaseEntity *pPlayer = GetCBaseEntity(params[1], true);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	T *pVar = GetPlayerVarAddressOrError<T>(varName, pContext, pPlayer);
	if (pVar)
	{	
		return (cell_t)*pVar;
	}

	return 0;
}

template <typename T>
static inline cell_t SetPlayerVar(IPluginContext *pContext, const cell_t *params, const char *varName)
{
	CBaseEntity *pPlayer = GetCBaseEntity(params[1], true);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	T *pVar = GetPlayerVarAddressOrError<T>(varName, pContext, pPlayer);
	if (pVar)
	{
		*pVar = (T)params[2];
	}

	return 0;
}

static cell_t CS_SetMVPCount(IPluginContext *pContext, const cell_t *params)
{
	return SetPlayerVar<int>(pContext, params, "MVPs");
}

static cell_t CS_GetMVPCount(IPluginContext *pContext, const cell_t *params)
{
	return GetPlayerVar<int>(pContext, params, "MVPs");
}

static cell_t CS_SetClientContributionScore(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	return SetPlayerVar<int>(pContext, params, "CScore");
#else
	return pContext->ThrowNativeError("SetClientContributionScore is not supported on this game");
#endif
}

static cell_t CS_GetClientContributionScore(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	return GetPlayerVar<int>(pContext, params, "CScore");
#else
	return pContext->ThrowNativeError("GetClientContributionScore is not supported on this game");
#endif
}

static cell_t CS_SetClientAssists(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	return SetPlayerVar<int>(pContext, params, "Assists");
#else
	return pContext->ThrowNativeError("SetClientAssists is not supported on this game");
#endif
}

static cell_t CS_GetClientAssists(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	return GetPlayerVar<int>(pContext, params, "Assists");
#else
	return pContext->ThrowNativeError("GetClientAssists is not supported on this game");
#endif
}

static cell_t CS_UpdateClientModel(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("SetModelFromClass", 
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
sp_nativeinfo_t g_CSNatives[] = 
{
	{"CS_RespawnPlayer",			CS_RespawnPlayer}, 
	{"CS_SwitchTeam",				CS_SwitchTeam},
	{"CS_DropWeapon",				CS_DropWeapon},
	{"CS_TerminateRound",			CS_TerminateRound},
	{"CS_GetTranslatedWeaponAlias",	CS_GetTranslatedWeaponAlias},
	{"CS_GetWeaponPrice",			CS_GetWeaponPrice},
	{"CS_GetClientClanTag",			CS_GetClientClanTag},
	{"CS_SetClientClanTag",			CS_SetClientClanTag},
	{"CS_AliasToWeaponID",			CS_AliasToWeaponID},
	{"CS_GetTeamScore",				CS_GetTeamScore},
	{"CS_SetTeamScore",				CS_SetTeamScore},
	{"CS_GetMVPCount",				CS_GetMVPCount},
	{"CS_SetMVPCount",				CS_SetMVPCount},
	{"CS_WeaponIDToAlias",			CS_WeaponIDToAlias},
	{"CS_GetClientContributionScore",	CS_GetClientContributionScore},
	{"CS_SetClientContributionScore",	CS_SetClientContributionScore},
	{"CS_GetClientAssists",			CS_GetClientAssists},
	{"CS_SetClientAssists",			CS_SetClientAssists},
	{"CS_UpdateClientModel",		CS_UpdateClientModel},
	{"CS_IsValidWeaponID",			CS_IsValidWeaponID},
	{NULL,							NULL}
};

