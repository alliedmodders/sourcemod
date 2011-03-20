/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Random Map Cycle Plugin
 * Randomly picks a map from the mapcycle.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

#pragma semicolon 1
#include <sourcemod>

public Plugin:myinfo =
{
	name = "RandomCycle",
	author = "AlliedModders LLC",
	description = "Randomly chooses the next map.",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_Cvar_ExcludeMaps = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_OldMapList = INVALID_HANDLE;
new g_mapListSerial = -1;

public OnPluginStart()
{
	new arraySize = ByteCountToCells(33);	
	g_MapList = CreateArray(arraySize);
	g_OldMapList = CreateArray(arraySize);

	g_Cvar_ExcludeMaps = CreateConVar("sm_randomcycle_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	
	AutoExecConfig(true, "randomcycle");
}

public OnConfigsExecuted()
{
	if (ReadMapList(g_MapList, 
					g_mapListSerial, 
					"randomcycle", 
					MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER)
		== INVALID_HANDLE)
	{
		if (g_mapListSerial == -1)
		{
			LogError("Unable to create a valid map list.");
		}
	}
	
	CreateTimer(5.0, Timer_RandomizeNextmap); // Small delay to give Nextmap time to complete OnMapStart()
}

public Action:Timer_RandomizeNextmap(Handle:timer)
{
	decl String:map[32];

	new bool:oldMaps = false;
	if (GetConVarInt(g_Cvar_ExcludeMaps) && GetArraySize(g_MapList) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		oldMaps = true;
	}
	
	new b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
	GetArrayString(g_MapList, b, map, sizeof(map));

	while (oldMaps && FindStringInArray(g_OldMapList, map) != -1)
	{
		b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
		GetArrayString(g_MapList, b, map, sizeof(map));
	}
	
	PushArrayString(g_OldMapList, map);
	SetNextMap(map);

	if (GetArraySize(g_OldMapList) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		RemoveFromArray(g_OldMapList, 0);
	}

	LogAction(-1, -1, "RandomCycle has chosen %s for the nextmap.", map);	

	return Plugin_Stop;
}