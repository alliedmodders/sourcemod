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

#include "util_cstrike.h"

#include "extension.h"
#include "RegNatives.h"
#include <iplayerinfo.h>

#define REGISTER_ADDR(name, defaultret, code) \
	void *addr; \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		g_pSM->LogError(myself, "Failed to lookup %s signature.", name); \
		return defaultret; \
	} \
	code; \
	g_RegNatives.Register(pWrapper);

#define GET_MEMSIG(name, defaultret) \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		g_pSM->LogError(myself, "Failed to lookup %s signature.", name); \
		return defaultret;\
	}

#if SOURCE_ENGINE == SE_CSGO

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

CEconItemView *GetEconItemView(void *pEntity, int iSlot)
{
	if (!pEntity)
		return NULL;

	static ICallWrapper *pWrapper = NULL;
	static int thisPtrOffset = -1;

	if (!pWrapper)
	{
		int offset = -1;
		int byteOffset = -1;
		void *pHandleCommandBuy = NULL;
		if (!g_pGameConf->GetOffset("GetItemInLoadout", &offset) || offset == -1)
		{
			smutils->LogError(myself, "Failed to get GetItemInLoadout offset.");
			return NULL;
		}
		else if (!g_pGameConf->GetOffset("CCSPlayerInventoryOffset", &byteOffset) || byteOffset == -1)
		{
			smutils->LogError(myself, "Failed to get CCSPlayerInventoryOffset offset.");
			return NULL;
		}
		else if (!g_pGameConf->GetMemSig("HandleCommand_Buy_Internal", &pHandleCommandBuy) || !pHandleCommandBuy)
		{
			smutils->LogError(myself, "Failed to get HandleCommand_Buy_Internal function.");
			return NULL;
		}
		else
		{
			thisPtrOffset = *(int *)((intptr_t)pHandleCommandBuy + byteOffset);

			PassInfo pass[2];
			PassInfo ret;
			pass[0].flags = PASSFLAG_BYVAL;
			pass[0].type = PassType_Basic;
			pass[0].size = sizeof(int);
			pass[1].flags = PASSFLAG_BYVAL;
			pass[1].type = PassType_Basic;
			pass[1].size = sizeof(int);

			ret.flags = PASSFLAG_BYVAL;
			ret.type = PassType_Basic;
			ret.size = sizeof(CEconItemView *);

			pWrapper = g_pBinTools->CreateVCall(offset, 0, 0, &ret, pass, 2);
			g_RegNatives.Register(pWrapper);
		}
	}

	int client = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(pEntity));

	IPlayerInfo *playerinfo = playerhelpers->GetGamePlayer(client)->GetPlayerInfo();
	
	if (!playerinfo)
		return NULL;

	int team = playerinfo->GetTeamIndex();

	if (team != 2 && team != 3)
		return NULL;

	CEconItemView *ret;
	unsigned char vstk[sizeof(void *) + sizeof(int) * 2];
	unsigned char *vptr = vstk;

	*(void **)vptr = (void *)((intptr_t)pEntity + thisPtrOffset);
	vptr += sizeof(void *);
	*(int *)vptr = team;
	vptr += sizeof(int);
	*(int *)vptr = iSlot;

	pWrapper->Execute(vstk, &ret);

	return ret;
}

void *GetCCSWeaponData(CEconItemView *view)
{
	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("GetCCSWeaponData", NULL,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(CEconItemView *); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(void *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, &retpass, pass, 1))
	}

	unsigned char vstk[sizeof(void *)];
	unsigned char *vptr = vstk;

	*(CEconItemView **)vptr = view;

	void *pWpnData = NULL;

	pWrapper->Execute(vstk, &pWpnData);

	return pWpnData;
}

void *GetItemSchema()
{
	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("GetItemSchema", NULL,
			PassInfo retpass; \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(void *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass, NULL, 0))
	}

	void *pSchema = NULL;
	pWrapper->Execute(NULL, &pSchema);

	//In windows this is actually ItemSystem() + 4 is ItemSchema
#ifdef WIN32
	return (void *)((intptr_t)pSchema + 4);
#else
	return pSchema;
#endif
}

void *GetItemDefintionByName(const char *classname)
{
	void *pSchema = GetItemSchema();

	if (!pSchema)
		return NULL;

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		int offset = -1;

		if (!g_pGameConf->GetOffset("GetItemDefintionByName", &offset) || offset == -1)
		{
			smutils->LogError(myself, "Failed to get GetItemDefintionByName offset.");
			return NULL;
		}

		PassInfo pass[1];
		PassInfo ret;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].type = PassType_Basic;
		pass[0].size = sizeof(const char *);

		ret.flags = PASSFLAG_BYVAL;
		ret.type = PassType_Basic;
		ret.size = sizeof(void *);

		pWrapper = g_pBinTools->CreateVCall(offset, 0, 0, &ret, pass, 1);

		g_RegNatives.Register(pWrapper);
	}

	unsigned char vstk[sizeof(void *) + sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(void **)vptr = pSchema;
	vptr += sizeof(void *);
	*(const char **)vptr = classname;

	void *pItemDef = NULL;
	pWrapper->Execute(vstk, &pItemDef);

	return pItemDef;
}

void *GetCCSWpnDataFromItemDef(void *pItemDef)
{
	if (!pItemDef)
		return NULL;

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		// In windows this is a sig to the inlined code in GetCCSWeaponData
		// We abuse the fact that the ItemDef is stored in ecx
		REGISTER_ADDR("GetCCSWeaponDataFromDef", NULL,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(void *); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(void *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, &retpass, pass, 1))
	}

	unsigned char vstk[sizeof(void *)];
	unsigned char *vptr = vstk;

	*(void **)vptr = pItemDef;

	void *pWpnData = NULL;
	pWrapper->Execute(vstk, &pWpnData);

	return pWpnData;
}
#endif

#if SOURCE_ENGINE != SE_CSGO
void *GetWeaponInfo(int weaponID)
{
	void *info;

	static ICallWrapper *pWrapper = NULL;
	if (!pWrapper)
	{
		REGISTER_ADDR("GetWeaponInfo", NULL,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(int); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(void *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass, pass, 1))
	}

	unsigned char vstk[sizeof(int)];
	unsigned char *vptr = vstk;

	*(int *)vptr = weaponID;

	pWrapper->Execute(vstk, &info);

	return info;
}
#endif

const char *GetTranslatedWeaponAlias(const char *weapon)
{
#if SOURCE_ENGINE != SE_CSGO
	const char *alias = NULL;

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("GetTranslatedWeaponAlias", weapon,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(const char *); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(const char *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass, pass, 1))
	}

	unsigned char vstk[sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(const char **)vptr = weapon;

	pWrapper->Execute(vstk, &alias);
	return alias;
#else //this should work for both games maybe replace both?

	static const char *szAliases[] =
	{
		"cv47", "ak47",
		"magnum", "awp",
		"d3au1", "g3sg1",
		"clarion", "famas",
		"bullpup", "aug",
		"9x19mm", "glock",
		"nighthawk", "deagle",
		"elites", "elite",
		"fn57", "fiveseven",
		"autoshotgun", "xm1014",
		"c90", "p90",
		"vest", "kevlar",
		"vesthelm", "assaultsuit",
		"nvgs", "nightvision"
	};
	
	for (int i = 0; i < SM_ARRAYSIZE(szAliases)/2; i++)
	{
		if (stricmp(weapon, szAliases[i * 2]) == 0)
			return szAliases[i * 2 + 1];
	}

	return weapon;
#endif
}

int AliasToWeaponID(const char *weapon)
{
#if SOURCE_ENGINE != SE_CSGO
	int weaponID = 0;

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("AliasToWeaponID", 0,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(const char *); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(int); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass, pass, 1))
	}

	unsigned char vstk[sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(const char **)vptr = weapon;

	pWrapper->Execute(vstk, &weaponID);

	return weaponID;
#else //this is horribly broken hackfix for now.
	for (unsigned int i = 0; i < SM_ARRAYSIZE(szWeaponInfo); i++)
	{
		if (stricmp(weapon, szWeaponInfo[i]) == 0)
			return i;
	}
	return 0;
#endif
}

const char *WeaponIDToAlias(int weaponID)
{
#if SOURCE_ENGINE != SE_CSGO
	const char *alias = NULL;

	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("WeaponIDToAlias", 0,
			PassInfo pass[1]; \
			PassInfo retpass; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].type = PassType_Basic; \
			pass[0].size = sizeof(int); \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(const char *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retpass, pass, 1))
	}
	
	unsigned char vstk[sizeof(int)];
	unsigned char *vptr = vstk;

	*(int *)vptr = GetRealWeaponID(weaponID);

	pWrapper->Execute(vstk, &alias);

	return alias;
#else
	int realID = GetRealWeaponID(weaponID);

	if (realID < SM_ARRAYSIZE(szWeaponInfo) && realID > 0)
		return szWeaponInfo[realID];

	return NULL;

#endif
	
}

int GetRealWeaponID(int weaponId)
{
#if SOURCE_ENGINE == SE_CSGO
	switch((SMCSWeapon)weaponId)
	{
		case SMCSWeapon_NONE:
			return (int)CSGOWeapon_NONE;
		case SMCSWeapon_DEAGLE:
			return (int)CSGOWeapon_DEAGLE;
		case SMCSWeapon_ELITE:
			return (int)CSGOWeapon_ELITE;
		case SMCSWeapon_FIVESEVEN:
			return (int)CSGOWeapon_FIVESEVEN;
		case SMCSWeapon_GLOCK:
			return (int)CSGOWeapon_GLOCK;
		case SMCSWeapon_P228:
			return (int)CSGOWeapon_P228;
		case SMCSWeapon_USP:
			return (int)CSGOWeapon_USP;
		case SMCSWeapon_AK47:
			return (int)CSGOWeapon_AK47;
		case SMCSWeapon_AUG:
			return (int)CSGOWeapon_AUG;
		case SMCSWeapon_AWP:
			return (int)CSGOWeapon_AWP;
		case SMCSWeapon_FAMAS:
			return (int)CSGOWeapon_FAMAS;
		case SMCSWeapon_G3SG1:
			return (int)CSGOWeapon_G3SG1;
		case SMCSWeapon_GALIL:
			return (int)CSGOWeapon_GALIL;
		case SMCSWeapon_GALILAR:
			return (int)CSGOWeapon_GALILAR;
		case SMCSWeapon_M249:
			return (int)CSGOWeapon_M249;
		case SMCSWeapon_M3:
			return (int)CSGOWeapon_M3;
		case SMCSWeapon_M4A1:
			return (int)CSGOWeapon_M4A1;
		case SMCSWeapon_MAC10:
			return (int)CSGOWeapon_MAC10;
		case SMCSWeapon_MP5NAVY:
			return (int)CSGOWeapon_MP5NAVY;
		case SMCSWeapon_P90:
			return (int)CSGOWeapon_P90;
		case SMCSWeapon_SCOUT:
			return (int)CSGOWeapon_SCOUT;
		case SMCSWeapon_SG550:
			return (int)CSGOWeapon_SG550;
		case SMCSWeapon_SG552:
			return (int)CSGOWeapon_SG552;
		case SMCSWeapon_TMP:
			return (int)CSGOWeapon_TMP;
		case SMCSWeapon_UMP45:
			return (int)CSGOWeapon_UMP45;
		case SMCSWeapon_XM1014:
			return (int)CSGOWeapon_XM1014;
		case SMCSWeapon_BIZON:
			return (int)CSGOWeapon_BIZON;
		case SMCSWeapon_MAG7:
			return (int)CSGOWeapon_MAG7;
		case SMCSWeapon_NEGEV:
			return (int)CSGOWeapon_NEGEV;
		case SMCSWeapon_SAWEDOFF:
			return (int)CSGOWeapon_SAWEDOFF;
		case SMCSWeapon_TEC9:
			return (int)CSGOWeapon_TEC9;
		case SMCSWeapon_TASER:
			return (int)CSGOWeapon_TASER;
		case SMCSWeapon_HKP2000:
			return (int)CSGOWeapon_HKP2000;
		case SMCSWeapon_MP7:
			return (int)CSGOWeapon_MP7;
		case SMCSWeapon_MP9:
			return (int)CSGOWeapon_MP9;
		case SMCSWeapon_NOVA:
			return (int)CSGOWeapon_NOVA;
		case SMCSWeapon_P250:
			return (int)CSGOWeapon_P250;
		case SMCSWeapon_SCAR17:
			return (int)CSGOWeapon_SCAR17;
		case SMCSWeapon_SCAR20:
			return (int)CSGOWeapon_SCAR20;
		case SMCSWeapon_SG556:
			return (int)CSGOWeapon_SG556;
		case SMCSWeapon_SSG08:
			return (int)CSGOWeapon_SSG08;
		case SMCSWeapon_KNIFE_GG:
			return (int)CSGOWeapon_KNIFE_GG;
		case SMCSWeapon_KNIFE:
			return (int)CSGOWeapon_KNIFE;
		case SMCSWeapon_FLASHBANG:
			return (int)CSGOWeapon_FLASHBANG;
		case SMCSWeapon_HEGRENADE:
			return (int)CSGOWeapon_HEGRENADE;
		case SMCSWeapon_SMOKEGRENADE:
			return (int)CSGOWeapon_SMOKEGRENADE;
		case SMCSWeapon_MOLOTOV:
			return (int)CSGOWeapon_MOLOTOV;
		case SMCSWeapon_DECOY:
			return (int)CSGOWeapon_DECOY;
		case SMCSWeapon_INCGRENADE:
			return (int)CSGOWeapon_INCGRENADE;
		case SMCSWeapon_C4:
			return (int)CSGOWeapon_C4;
		case SMCSWeapon_KEVLAR:
			return (int)CSGOWeapon_KEVLAR;
		case SMCSWeapon_ASSAULTSUIT:
			return (int)CSGOWeapon_ASSAULTSUIT;
		case SMCSWeapon_NIGHTVISION:
			return (int)CSGOWeapon_NVG;
		case SMCSWeapon_DEFUSER:
			return (int)CSGOWeapon_DEFUSER;
		default:
			return (int)CSGOWeapon_NONE;
	}
#else
	if (weaponId > (int)SMCSWeapon_NIGHTVISION || weaponId < (int)SMCSWeapon_NONE)
		return (int)SMCSWeapon_NONE;
	else
		return weaponId;
#endif
}

int GetFakeWeaponID(int weaponId)
{
#if SOURCE_ENGINE == SE_CSGO
	switch((CSGOWeapon)weaponId)
	{
		case CSGOWeapon_NONE:
			return (int)SMCSWeapon_NONE;
		case CSGOWeapon_DEAGLE:
			return (int)SMCSWeapon_DEAGLE;
		case CSGOWeapon_ELITE:
			return (int)SMCSWeapon_ELITE;
		case CSGOWeapon_FIVESEVEN:
			return (int)SMCSWeapon_FIVESEVEN;
		case CSGOWeapon_GLOCK:
			return (int)SMCSWeapon_GLOCK;
		case CSGOWeapon_P228:
			return (int)SMCSWeapon_P228;
		case CSGOWeapon_USP:
			return (int)SMCSWeapon_USP;
		case CSGOWeapon_AK47:
			return (int)SMCSWeapon_AK47;
		case CSGOWeapon_AUG:
			return (int)SMCSWeapon_AUG;
		case CSGOWeapon_AWP:
			return (int)SMCSWeapon_AWP;
		case CSGOWeapon_FAMAS:
			return (int)SMCSWeapon_FAMAS;
		case CSGOWeapon_G3SG1:
			return (int)SMCSWeapon_G3SG1;
		case CSGOWeapon_GALIL:
			return (int)SMCSWeapon_GALIL;
		case CSGOWeapon_GALILAR:
			return (int)SMCSWeapon_GALILAR;
		case CSGOWeapon_M249:
			return (int)SMCSWeapon_M249;
		case CSGOWeapon_M3:
			return (int)SMCSWeapon_M3;
		case CSGOWeapon_M4A1:
			return (int)SMCSWeapon_M4A1;
		case CSGOWeapon_MAC10:
			return (int)SMCSWeapon_MAC10;
		case CSGOWeapon_MP5NAVY:
			return (int)SMCSWeapon_MP5NAVY;
		case CSGOWeapon_P90:
			return (int)SMCSWeapon_P90;
		case CSGOWeapon_SCOUT:
			return (int)SMCSWeapon_SCOUT;
		case CSGOWeapon_SG550:
			return (int)SMCSWeapon_SG550;
		case CSGOWeapon_SG552:
			return (int)SMCSWeapon_SG552;
		case CSGOWeapon_TMP:
			return (int)SMCSWeapon_TMP;
		case CSGOWeapon_UMP45:
			return (int)SMCSWeapon_UMP45;
		case CSGOWeapon_XM1014:
			return (int)SMCSWeapon_XM1014;
		case CSGOWeapon_BIZON:
			return (int)SMCSWeapon_BIZON;
		case CSGOWeapon_MAG7:
			return (int)SMCSWeapon_MAG7;
		case CSGOWeapon_NEGEV:
			return (int)SMCSWeapon_NEGEV;
		case CSGOWeapon_SAWEDOFF:
			return (int)SMCSWeapon_SAWEDOFF;
		case CSGOWeapon_TEC9:
			return (int)SMCSWeapon_TEC9;
		case CSGOWeapon_TASER:
			return (int)SMCSWeapon_TASER;
		case CSGOWeapon_HKP2000:
			return (int)SMCSWeapon_HKP2000;
		case CSGOWeapon_MP7:
			return (int)SMCSWeapon_MP7;
		case CSGOWeapon_MP9:
			return (int)SMCSWeapon_MP9;
		case CSGOWeapon_NOVA:
			return (int)SMCSWeapon_NOVA;
		case CSGOWeapon_P250:
			return (int)SMCSWeapon_P250;
		case CSGOWeapon_SCAR17:
			return (int)SMCSWeapon_SCAR17;
		case CSGOWeapon_SCAR20:
			return (int)SMCSWeapon_SCAR20;
		case CSGOWeapon_SG556:
			return (int)SMCSWeapon_SG556;
		case CSGOWeapon_SSG08:
			return (int)SMCSWeapon_SSG08;
		case CSGOWeapon_KNIFE_GG:
			return (int)SMCSWeapon_KNIFE_GG;
		case CSGOWeapon_KNIFE:
			return (int)SMCSWeapon_KNIFE;
		case CSGOWeapon_FLASHBANG:
			return (int)SMCSWeapon_FLASHBANG;
		case CSGOWeapon_HEGRENADE:
			return (int)SMCSWeapon_HEGRENADE;
		case CSGOWeapon_SMOKEGRENADE:
			return (int)SMCSWeapon_SMOKEGRENADE;
		case CSGOWeapon_MOLOTOV:
			return (int)SMCSWeapon_MOLOTOV;
		case CSGOWeapon_DECOY:
			return (int)SMCSWeapon_DECOY;
		case CSGOWeapon_INCGRENADE:
			return (int)SMCSWeapon_INCGRENADE;
		case CSGOWeapon_C4:
			return (int)SMCSWeapon_C4;
		case CSGOWeapon_KEVLAR:
			return (int)SMCSWeapon_KEVLAR;
		case CSGOWeapon_ASSAULTSUIT:
			return (int)SMCSWeapon_ASSAULTSUIT;
		case CSGOWeapon_NVG:
			return (int)SMCSWeapon_NIGHTVISION;
		case CSGOWeapon_DEFUSER:
			return (int)SMCSWeapon_DEFUSER;
		default:
			return (int)SMCSWeapon_NONE;
	}
#else
	if (weaponId > (int)SMCSWeapon_NIGHTVISION || weaponId < (int)SMCSWeapon_NONE)
		return (int)SMCSWeapon_NONE;
	else
		return weaponId;
#endif
}
bool IsValidWeaponID(int id)
{
	if (id <= (int)SMCSWeapon_NONE)
		return false;
	//Why are these even HERE!?! They dont exist in CS:GO but have valid ID's still
#if SOURCE_ENGINE == SE_CSGO
	else if (id > (int)SMCSWeapon_DEFUSER || id == (int)SMCSWeapon_P228 || id == (int)SMCSWeapon_SCOUT || id == (int)SMCSWeapon_SG550 || id == (int)SMCSWeapon_GALIL ||
			id == (int)SMCSWeapon_SCAR17 || id == (int)SMCSWeapon_USP || id == (int)SMCSWeapon_M3 || id == (int)SMCSWeapon_MP5NAVY || id == (int)SMCSWeapon_TMP || id == (int)SMCSWeapon_SG552)
			return false;
#else
	else if (id > (int)SMCSWeapon_NIGHTVISION)
		return false;
#endif
	return true;
}
