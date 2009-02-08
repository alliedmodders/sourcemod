/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _tf2_stocks_included
 #endinput
#endif
#define _tf2_stocks_included

#include <tf2>
#include <sdktools>

enum TFResourceType
{
	TFResource_Ping,
	TFResource_Score,
	TFResource_Deaths,
	TFResource_TotalScore, 
	TFResource_Captures,
	TFResource_Defenses,
	TFResource_Dominations,
	TFResource_Revenge,
	TFResource_BuildingsDestroyed, 
	TFResource_Headshots,
	TFResource_Backstabs,
	TFResource_HealPoints,
	TFResource_Invulns,
	TFResource_Teleports,
	TFResource_ResupplyPoints,
	TFResource_KillAssists,
	TFResource_MaxHealth,
	TFResource_PlayerClass
};

static const String:TFResourceNames[TFResourceType][] =
{
	"m_iPing",
	"m_iScore",
	"m_iDeaths",
	"m_iTotalScore",
	"m_iCaptures",
	"m_iDefenses",
	"m_iDominations",
	"m_iRevenge",
	"m_iBuildingsDestroyed",
	"m_iHeadshots",
	"m_iBackstabs",
	"m_iHealPoints",
	"m_iInvulns",
	"m_iTeleports",
	"m_iResupplyPoints",
	"m_iKillAssists",
	"m_iMaxHealth",
	"m_iPlayerClass"
};

/**
 * Get's a Clients current class.
 *
 * @param client		Player's index.
 * @return				Current TFClassType of player.
 * @error				Invalid client index.
 */
stock TFClassType:TF2_GetPlayerClass(client)
{
	return TFClassType:GetEntProp(client, Prop_Send, "m_iClass");
}

/**
 * Set's a Clients class.
 *
 * Note: If setting player class in a player spawn hook weapons should be set to false.
 *
 * @param client		Player's index.
 * @param class			TFClassType class symbol.
 * @param weapons		This paramater is ignored.
 * @param persistant	If true changes the players desired class so the change stays after death.
 * @noreturn
 * @error				Invalid client index.
 */
stock TF2_SetPlayerClass(client, TFClassType:class, bool:weapons=true, bool:persistant=true)
{
	SetEntProp(client, Prop_Send, "m_iClass", _:class);
	
	if (persistant)
	{
		SetEntProp(client, Prop_Send, "m_iDesiredPlayerClass", _:class);
	}
}

/**
 * Retrieves client data from the resource entity
 *
 * @param client		Player's index.
 * @param type			ResourceType constant
 * @return				Value or -1 on failure.
 * @error				Invalid client index, client not in game or failed to find resource entity.
 */
stock TF2_GetPlayerResourceData(client, TFResourceType:type)
{
	if (!IsClientConnected(client))
	{
		return -1;	
	}
	
	new offset = FindSendPropInfo("CTFPlayerResource", TFResourceNames[type]);
		
	if (offset < 1)
	{
		return -1;	
	}
				
	new entity = TF2_GetResourceEntity();
	
	if (entity == -1)
	{
		return -1;
	}
	
	return GetEntData(entity, offset + (client*4));
}

/**
 * Sets client data in the resource entity
 *
 * Note: The game overwrites these values every frame, so changing them will have very little effect.
 *
 * @param client		Player's index.
 * @param type			ResourceType constant
 * @param value			Value to set.
 * @return				Value or -1 on failure.
 * @error				Invalid client index, client not in game or failed to find resource entity.
 */
stock bool:TF2_SetPlayerResourceData(client, TFResourceType:type, any:value)
{
	if (!IsClientConnected(client))
	{
		return false;	
	}
	
	new offset = FindSendPropInfo("CTFPlayerResource", TFResourceNames[type]);
						
	if (offset < 1)
	{
		return false;	
	}
				
	new entity = TF2_GetResourceEntity();
	
	if (entity == -1)
	{
		return false;	
	}
	
	SetEntData(entity, offset + (client*4), value);
	
	return true;	
}

/**
 * Removes all weapons from a client's weapon slot
 *
 * @param client		Player's index.
 * @param slot			Slot index (0-5)
 * @noreturn
 * @error				Invalid client, invalid slot or lack of mod support
 */
stock TF2_RemoveWeaponSlot(client, slot)
{
	new weaponIndex;
	while ((weaponIndex = GetPlayerWeaponSlot(client, slot)) != -1)
	{
		RemovePlayerItem(client, weaponIndex);
		RemoveEdict(weaponIndex);
	}
}

/**
 * Removes all weapons from a client
 *
 * @param client		Player's index.
 * @noreturn
 */
stock TF2_RemoveAllWeapons(client)
{
	for (new i = 0; i <= 5; i++)
	{
		TF2_RemoveWeaponSlot(client, i);
	}
}

