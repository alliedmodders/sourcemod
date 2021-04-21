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
#include "util_cstrike.h"
#include "RegNatives.h"
#include <iplayerinfo.h>
#include <sm_argbuffer.h>

#if SOURCE_ENGINE == SE_CSGO
#include "itemdef-hash.h"

ClassnameMap g_mapClassToDefIdx;
ItemIndexMap g_mapDefIdxToClass;
WeaponIDMap g_mapWeaponIDToDefIdx;
#endif

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

CEconItemView *GetEconItemView(CBaseEntity *pEntity, int iSlot)
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

	int client = gamehelpers->EntityToBCompatRef(pEntity);

	IPlayerInfo *playerinfo = playerhelpers->GetGamePlayer(client)->GetPlayerInfo();
	
	if (!playerinfo)
		return NULL;

	int team = playerinfo->GetTeamIndex();

	if (team != 2 && team != 3)
		return NULL;

	ArgBuffer<void*, int, int> vstk(reinterpret_cast<void*>(((intptr_t)pEntity + thisPtrOffset)), team, iSlot);

	CEconItemView *ret = nullptr;
	pWrapper->Execute(vstk, &ret);

	return ret;
}

CCSWeaponData *GetCCSWeaponData(CEconItemView *view)
{
	static ICallWrapper *pWrapper = NULL;

	if (!pWrapper)
	{
		REGISTER_ADDR("GetCCSWeaponData", NULL,
			PassInfo retpass; \
			retpass.flags = PASSFLAG_BYVAL; \
			retpass.type = PassType_Basic; \
			retpass.size = sizeof(CCSWeaponData *); \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, &retpass, NULL, 0))
	}

	ArgBuffer<CEconItemView*> vstk(view);

	CCSWeaponData *pWpnData = nullptr;
	pWrapper->Execute(vstk, &pWpnData);

	return pWpnData;
}

CEconItemSchema *GetItemSchema()
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

	//On windows/mac this is actually ItemSystem() + sizeof(void *) is ItemSchema
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_APPLE)
	return (CEconItemSchema *)((intptr_t)pSchema + sizeof(void *));
#else
	return (CEconItemSchema *)pSchema;
#endif
}

CEconItemDefinition *GetItemDefintionByName(const char *classname)
{
	CEconItemSchema *pSchema = GetItemSchema();

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
		ret.size = sizeof(CEconItemDefinition *);

		pWrapper = g_pBinTools->CreateVCall(offset, 0, 0, &ret, pass, 1);

		g_RegNatives.Register(pWrapper);
	}

	ArgBuffer<void*, const char *> vstk(pSchema, classname);

	CEconItemDefinition *pItemDef = nullptr;
	pWrapper->Execute(vstk, &pItemDef);

	return pItemDef;
}

void CreateHashMaps()
{
	CEconItemSchema *pSchema = GetItemSchema();

	if (!pSchema)
		return;

	static const char *pPriceKey = NULL;

	if (!pPriceKey)
	{
		pPriceKey = g_pGameConf->GetKeyValue("PriceKey");
		if (!pPriceKey)
		{
			return;
		}
	}

	static int iHashMapOffset = -1;

	if (iHashMapOffset == -1)
	{
		if (!g_pGameConf->GetOffset("ItemDefHashOffset", &iHashMapOffset) || iHashMapOffset == -1)
		{
			return;
		}
	}

	g_mapClassToDefIdx.init();
	g_mapDefIdxToClass.init();
	g_mapWeaponIDToDefIdx.init();

	CHashItemDef *map = (CHashItemDef *)((intptr_t)pSchema + iHashMapOffset);

	for (int i = 0; i < map->currentElements; i++)
	{
		HashItemDef_Node node = map->pMem[i];

		if (!node.pDef || !node.pDef->m_pKv)
			continue;

		KeyValues *pClassname = node.pDef->m_pKv->FindKey("name", false);
		KeyValues *pAttributes = node.pDef->m_pKv->FindKey("attributes", false);

		if (pClassname && pAttributes)
		{
			KeyValues *pPrice = pAttributes->FindKey(pPriceKey, false);

			if (pPrice)
			{
				const char *classname = pClassname->GetString();

				unsigned int price = pPrice->GetInt();
				uint16_t iItemDefIdx = node.pDef->m_iDefinitionIndex;
				SMCSWeapon iWeaponID = GetWeaponIdFromDefIdx(iItemDefIdx);
				int iLoadoutslot = node.pDef->GetDefaultLoadoutSlot();

				ClassnameMap::Insert i = g_mapClassToDefIdx.findForAdd(classname);
				g_mapClassToDefIdx.add(i, std::string(classname), ItemDefHashValue(iLoadoutslot, price, iWeaponID, iItemDefIdx, classname));

				ItemIndexMap::Insert x = g_mapDefIdxToClass.findForAdd(iItemDefIdx);
				g_mapDefIdxToClass.add(x, iItemDefIdx, ItemDefHashValue(iLoadoutslot, price, iWeaponID, iItemDefIdx, classname));

				WeaponIDMap::Insert t = g_mapWeaponIDToDefIdx.findForAdd(iWeaponID);
				g_mapWeaponIDToDefIdx.add(t, iWeaponID, ItemDefHashValue(iLoadoutslot, price, iWeaponID, iItemDefIdx, classname));
			}
		}
	}
}

void ClearHashMaps()
{
	g_mapClassToDefIdx.clear();
	g_mapDefIdxToClass.clear();
	g_mapWeaponIDToDefIdx.clear();
}

SMCSWeapon GetWeaponIdFromDefIdx(uint16_t iDefIdx)
{
	//DEAR GOD THIS IS HIDEOUS
	//None in the middle are weapons that dont exist.
	//If they are added and use the same idx they should be changed to their respective ones
	static SMCSWeapon weaponIDMap[SMCSWeapon_MAXWEAPONIDS] =
	{
		SMCSWeapon_NONE, SMCSWeapon_DEAGLE, SMCSWeapon_ELITE, SMCSWeapon_FIVESEVEN,
		SMCSWeapon_GLOCK, SMCSWeapon_NONE, SMCSWeapon_NONE, SMCSWeapon_AK47,
		SMCSWeapon_AUG, SMCSWeapon_AWP, SMCSWeapon_FAMAS, SMCSWeapon_G3SG1,
		SMCSWeapon_NONE, SMCSWeapon_GALILAR, SMCSWeapon_M249, SMCSWeapon_NONE,
		SMCSWeapon_M4A1, SMCSWeapon_MAC10, SMCSWeapon_NONE, SMCSWeapon_P90,
		SMCSWeapon_NONE, SMCSWeapon_NONE, SMCSWeapon_NONE, SMCSWeapon_MP5NAVY,
		SMCSWeapon_UMP45, SMCSWeapon_XM1014, SMCSWeapon_BIZON, SMCSWeapon_MAG7,
		SMCSWeapon_NEGEV, SMCSWeapon_SAWEDOFF, SMCSWeapon_TEC9, SMCSWeapon_TASER,
		SMCSWeapon_HKP2000, SMCSWeapon_MP7, SMCSWeapon_MP9, SMCSWeapon_NOVA,
		SMCSWeapon_P250, SMCSWeapon_SHIELD, SMCSWeapon_SCAR20, SMCSWeapon_SG556,
		SMCSWeapon_SSG08, SMCSWeapon_KNIFE_GG, SMCSWeapon_KNIFE, SMCSWeapon_FLASHBANG,
		SMCSWeapon_HEGRENADE, SMCSWeapon_SMOKEGRENADE, SMCSWeapon_MOLOTOV, SMCSWeapon_DECOY,
		SMCSWeapon_INCGRENADE, SMCSWeapon_C4, SMCSWeapon_KEVLAR, SMCSWeapon_ASSAULTSUIT,
		SMCSWeapon_HEAVYASSAULTSUIT, SMCSWeapon_NONE, SMCSWeapon_NIGHTVISION, SMCSWeapon_DEFUSER
	};

	if (iDefIdx >= SMCSWeapon_MAXWEAPONIDS)
		return (SMCSWeapon)iDefIdx;
	else
		return weaponIDMap[iDefIdx];
}

ItemDefHashValue *GetHashValueFromWeapon(const char *szWeapon)
{
	char tempWeapon[MAX_WEAPON_NAME_LENGTH];

	ke::SafeStrcpy(tempWeapon, sizeof(tempWeapon), szWeapon);
	Q_strlower(tempWeapon);

	if (strstr(tempWeapon, "weapon_") == NULL && strstr(tempWeapon, "item_") == NULL)
	{
		static const char *szClassPrefixs[] = { "weapon_", "item_" };

		for (unsigned int i = 0; i < SM_ARRAYSIZE(szClassPrefixs); i++)
		{
			char classname[MAX_WEAPON_NAME_LENGTH];
			ke::SafeSprintf(classname, sizeof(classname), "%s%s", szClassPrefixs[i], tempWeapon);

			ClassnameMap::Result res = g_mapClassToDefIdx.find(classname);

			if (res.found())
				return &res->value;
		}

		return NULL;
	}

	ClassnameMap::Result res = g_mapClassToDefIdx.find(tempWeapon);

	if (res.found())
		return &res->value;

	return NULL;
}
#endif

#if SOURCE_ENGINE != SE_CSGO
void *GetWeaponInfo(int weaponID)
{
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

	ArgBuffer<int> vstk(weaponID);

	void *info = nullptr;
	pWrapper->Execute(vstk, &info);

	return info;
}
#endif

const char *GetWeaponNameFromClassname(const char *weapon)
{
	char *szTemp = strstr((char *)weapon, "_");

	if (!szTemp)
	{
		return weapon;
	}
	else
	{
		return (const char *)((intptr_t)szTemp + 1);
	}
}

const char *GetTranslatedWeaponAlias(const char *weapon)
{
#if SOURCE_ENGINE != SE_CSGO

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

	ArgBuffer<const char *> vstk(GetWeaponNameFromClassname(weapon));

	const char *alias = nullptr;
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

	for (size_t i = 0; i < SM_ARRAYSIZE(szAliases) / 2; i++)
	{
		if (Q_stristr(GetWeaponNameFromClassname(weapon), szAliases[i * 2]) != 0)
			return szAliases[i * 2 + 1];
	}

	return  GetWeaponNameFromClassname(weapon);
#endif
}

int AliasToWeaponID(const char *weapon)
{
#if SOURCE_ENGINE != SE_CSGO
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

	ArgBuffer<const char *> vstk(GetWeaponNameFromClassname(weapon));

	int weaponID = 0;
	pWrapper->Execute(vstk, &weaponID);

	return weaponID;
#else
	ItemDefHashValue *pHashValue = GetHashValueFromWeapon(weapon);

	if (pHashValue)
		return pHashValue->m_iWeaponID;

	return 0;
#endif
}

const char *WeaponIDToAlias(int weaponID)
{
#if SOURCE_ENGINE != SE_CSGO

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

	ArgBuffer<int> vstk(weaponID);

	const char *alias = nullptr;
	pWrapper->Execute(vstk, &alias);

	return alias;
#else
	WeaponIDMap::Result res = g_mapWeaponIDToDefIdx.find((SMCSWeapon)weaponID);

	if (res.found())
		return res->value.m_szItemName;

	return NULL;
#endif
}

bool IsValidWeaponID(int id)
{
	if (id <= (int)SMCSWeapon_NONE)
		return false;
#if SOURCE_ENGINE == SE_CSGO
	WeaponIDMap::Result res = g_mapWeaponIDToDefIdx.find((SMCSWeapon)id);
	if (!res.found())
		return false;
#else
	else if (id > SMCSWeapon_NIGHTVISION || !GetWeaponInfo(id))
		return false;
#endif
	return true;
}
