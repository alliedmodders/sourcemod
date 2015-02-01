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

#if SOURCE_ENGINE == SE_CSGO
class CEconItemView;

enum CSGOWeapon
{
	CSGOWeapon_NONE,
	CSGOWeapon_DEAGLE,
	CSGOWeapon_ELITE,
	CSGOWeapon_FIVESEVEN,
	CSGOWeapon_GLOCK,
	CSGOWeapon_P228,
	CSGOWeapon_USP,
	CSGOWeapon_AK47,
	CSGOWeapon_AUG,
	CSGOWeapon_AWP,
	CSGOWeapon_FAMAS,
	CSGOWeapon_G3SG1,
	CSGOWeapon_GALIL,
	CSGOWeapon_GALILAR,
	CSGOWeapon_M249,
	CSGOWeapon_M3,
	CSGOWeapon_M4A1,
	CSGOWeapon_MAC10,
	CSGOWeapon_MP5NAVY,
	CSGOWeapon_P90,
	CSGOWeapon_SCOUT,
	CSGOWeapon_SG550,
	CSGOWeapon_SG552,
	CSGOWeapon_TMP,
	CSGOWeapon_UMP45,
	CSGOWeapon_XM1014,
	CSGOWeapon_BIZON,
	CSGOWeapon_MAG7,
	CSGOWeapon_NEGEV,
	CSGOWeapon_SAWEDOFF,
	CSGOWeapon_TEC9,
	CSGOWeapon_TASER,
	CSGOWeapon_HKP2000,
	CSGOWeapon_MP7,
	CSGOWeapon_MP9,
	CSGOWeapon_NOVA,
	CSGOWeapon_P250,
	CSGOWeapon_SCAR17,
	CSGOWeapon_SCAR20,
	CSGOWeapon_SG556,
	CSGOWeapon_SSG08,
	CSGOWeapon_KNIFE_GG,
	CSGOWeapon_KNIFE,
	CSGOWeapon_FLASHBANG,
	CSGOWeapon_HEGRENADE,
	CSGOWeapon_SMOKEGRENADE,
	CSGOWeapon_MOLOTOV,
	CSGOWeapon_DECOY,
	CSGOWeapon_INCGRENADE,
	CSGOWeapon_C4, //49
	CSGOWeapon_KEVLAR = 50,
	CSGOWeapon_ASSAULTSUIT,
	CSGOWeapon_NVG,
	CSGOWeapon_DEFUSER
};
#endif
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
	SMCSWeapon_DEFUSER
};
void *GetWeaponInfo(int weaponID);

const char *GetTranslatedWeaponAlias(const char *weapon);

int AliasToWeaponID(const char *weapon);

const char *WeaponIDToAlias(int weaponID);

int GetRealWeaponID(int weaponId);

int GetFakeWeaponID(int weaponId);

bool IsValidWeaponID(int weaponId);

void *GetWeaponPriceFunction();

#endif
