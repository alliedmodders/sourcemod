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

#ifndef _INCLUDE_CSTRIKE_UTIL_H_
#define _INCLUDE_CSTRIKE_UTIL_H_
 //THIS IS THE INCLUDE ENUM DO NOT CHANGE ONLY UPDATE THE INCLUDE
 //This is used to match to old weaponid's to their correct enum value
 //Anything after heavy assault suit will pass the itemdef as they will be the id set in include
enum SMCSWeapon
{
	SMCSWeapon_NONE = 0,
	SMCSWeapon_P228,
	SMCSWeapon_GLOCK,
	SMCSWeapon_SCOUT,
	SMCSWeapon_HEGRENADE,
	SMCSWeapon_XM1014,
	SMCSWeapon_C4,
	SMCSWeapon_MAC10,
	SMCSWeapon_AUG,
	SMCSWeapon_SMOKEGRENADE,
	SMCSWeapon_ELITE,
	SMCSWeapon_FIVESEVEN,
	SMCSWeapon_UMP45,
	SMCSWeapon_SG550,
	SMCSWeapon_GALIL,
	SMCSWeapon_FAMAS,
	SMCSWeapon_USP,
	SMCSWeapon_AWP,
	SMCSWeapon_MP5NAVY,
	SMCSWeapon_M249,
	SMCSWeapon_M3,
	SMCSWeapon_M4A1,
	SMCSWeapon_TMP,
	SMCSWeapon_G3SG1,
	SMCSWeapon_FLASHBANG,
	SMCSWeapon_DEAGLE,
	SMCSWeapon_SG552,
	SMCSWeapon_AK47,
	SMCSWeapon_KNIFE,
	SMCSWeapon_P90,
	SMCSWeapon_SHIELD,
	SMCSWeapon_KEVLAR,
	SMCSWeapon_ASSAULTSUIT,
	SMCSWeapon_NIGHTVISION,
	SMCSWeapon_GALILAR,
	SMCSWeapon_BIZON,
	SMCSWeapon_MAG7,
	SMCSWeapon_NEGEV,
	SMCSWeapon_SAWEDOFF,
	SMCSWeapon_TEC9,
	SMCSWeapon_TASER,
	SMCSWeapon_HKP2000,
	SMCSWeapon_MP7,
	SMCSWeapon_MP9,
	SMCSWeapon_NOVA,
	SMCSWeapon_P250,
	SMCSWeapon_SCAR17,
	SMCSWeapon_SCAR20,
	SMCSWeapon_SG556,
	SMCSWeapon_SSG08,
	SMCSWeapon_KNIFE_GG,
	SMCSWeapon_MOLOTOV,
	SMCSWeapon_DECOY,
	SMCSWeapon_INCGRENADE,
	SMCSWeapon_DEFUSER,
	SMCSWeapon_HEAVYASSAULTSUIT,
	SMCSWeapon_MAXWEAPONIDS, //This only exists here... the include has more. This is for easy array construction
};

#if SOURCE_ENGINE == SE_CSGO
//These are the ItemDefintion indexs they are used as a reference to create GetWeaponIdFromDefIdx
/*
enum CSGOItemDefs
{
	CSGOItemDef_NONE = 0,
	CSGOItemDef_DEAGLE,
	CSGOItemDef_ELITE,
	CSGOItemDef_FIVESEVEN,
	CSGOItemDef_GLOCK,
	CSGOItemDef_P228,
	CSGOItemDef_USP,
	CSGOItemDef_AK47,
	CSGOItemDef_AUG,
	CSGOItemDef_AWP,
	CSGOItemDef_FAMAS,
	CSGOItemDef_G3SG1,
	CSGOItemDef_GALIL,
	CSGOItemDef_GALILAR,
	CSGOItemDef_M249,
	CSGOItemDef_M3,
	CSGOItemDef_M4A1,
	CSGOItemDef_MAC10,
	CSGOItemDef_MP5NAVY,
	CSGOItemDef_P90,
	CSGOItemDef_SCOUT,
	CSGOItemDef_SG550,
	CSGOItemDef_SG552,
	CSGOItemDef_TMP,
	CSGOItemDef_UMP45,
	CSGOItemDef_XM1014,
	CSGOItemDef_BIZON,
	CSGOItemDef_MAG7,
	CSGOItemDef_NEGEV,
	CSGOItemDef_SAWEDOFF,
	CSGOItemDef_TEC9,
	CSGOItemDef_TASER,
	CSGOItemDef_HKP2000,
	CSGOItemDef_MP7,
	CSGOItemDef_MP9,
	CSGOItemDef_NOVA,
	CSGOItemDef_P250,
	CSGOItemDef_SCAR17,
	CSGOItemDef_SCAR20,
	CSGOItemDef_SG556,
	CSGOItemDef_SSG08,
	CSGOItemDef_KNIFE_GG,
	CSGOItemDef_KNIFE,
	CSGOItemDef_FLASHBANG,
	CSGOItemDef_HEGRENADE,
	CSGOItemDef_SMOKEGRENADE,
	CSGOItemDef_MOLOTOV,
	CSGOItemDef_DECOY,
	CSGOItemDef_INCGRENADE,
	CSGOItemDef_C4,
	CSGOItemDef_KEVLAR,
	CSGOItemDef_ASSAULTSUIT,
	CSGOItemDef_HEAVYASSAULTSUIT,
	CSGOItemDef_UNUSED,
	CSGOItemDef_NVG,
	CSGOItemDef_DEFUSER,
	CSGOItemDef_MAXDEFS,
};
*/
struct ItemDefHashValue;
class CEconItemView;
class CCSWeaponData;
class CEconItemSchema;
class CEconItemDefinition
{
public:
	void **m_pVtable;
	KeyValues *m_pKv;
	uint16_t m_iDefinitionIndex;
	int GetDefaultLoadoutSlot()
	{
		static int iLoadoutSlotOffset = -1;

		if (iLoadoutSlotOffset == -1)
		{
			if (!g_pGameConf->GetOffset("LoadoutSlotOffset", &iLoadoutSlotOffset) || iLoadoutSlotOffset == -1)
			{
				iLoadoutSlotOffset = -1;
				return -1;
			}
		}

		return *(int *)((intptr_t)this + iLoadoutSlotOffset);
	}
};

CEconItemView *GetEconItemView(CBaseEntity *pEntity, int iSlot);
CCSWeaponData *GetCCSWeaponData(CEconItemView *view);
CEconItemSchema *GetItemSchema();
CEconItemDefinition *GetItemDefintionByName(const char *classname);
void CreateHashMaps();
void ClearHashMaps();
SMCSWeapon GetWeaponIdFromDefIdx(uint16_t iDefIdx);
ItemDefHashValue *GetHashValueFromWeapon(const char *szWeapon);
#else //CS:S ONLY STUFF
void *GetWeaponInfo(int weaponID);
#endif

const char *GetWeaponNameFromClassname(const char *weapon);
const char *GetTranslatedWeaponAlias(const char *weapon);
int AliasToWeaponID(const char *weapon);
const char *WeaponIDToAlias(int weaponID);
bool IsValidWeaponID(int weaponId);
#endif
