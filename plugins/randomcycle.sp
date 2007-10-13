/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Random Map Cycle Plugin
 * Randomly picks a map from the mapcycle.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

new Handle:g_Cvar_Nextmap = INVALID_HANDLE;
new Handle:g_Cvar_Mapfile = INVALID_HANDLE;
new Handle:g_Cvar_ExcludeMaps = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_OldMapList = INVALID_HANDLE;
new g_mapFileTime;

public OnPluginStart()
{
	g_MapList = CreateArray(33);
	g_OldMapList = CreateArray(33);

	g_Cvar_Mapfile = CreateConVar("sm_randomcycle_file", "configs/maps.ini", "Map file to use. (Def sourcemod/configs/maps.ini)");
	g_Cvar_ExcludeMaps = CreateConVar("sm_randomcycle_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	
	AutoExecConfig(true, "randomcycle");
}

public OnMapStart()
{
	g_Cvar_Nextmap = FindConVar("sm_nextmap");

	if (g_Cvar_Nextmap == INVALID_HANDLE)
	{
		LogError("FATAL: Cannot find sm_nextmap cvar. RandomCycle not loaded.");
		SetFailState("sm_nextmap not found");
	}
	
	if (LoadMaps())
	{
		CreateTimer(5.0, Timer_RandomizeNextmap); // Small delay to give Nextmap time to complete OnMapStart()
	}
}

public Action:Timer_RandomizeNextmap(Handle:timer)
{
	decl String:map[32];

	new bool:oldMaps = false;
	if (GetArraySize(g_MapList) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		oldMaps = true;
	}
	
	new b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
	GetArrayString(g_MapList, b, map, sizeof(map));

	while (oldMaps && IsStringInArray(g_OldMapList, map))
	{
		b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
		GetArrayString(g_MapList, b, map, sizeof(map));
	}
	
	SetConVarString(g_Cvar_Nextmap, map);
	PushArrayString(g_OldMapList, map);

	if (GetArraySize(g_OldMapList) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		RemoveFromArray(g_OldMapList, 0);
	}

	LogMessage("RandomCycle has chosen %s for the nextmap.", map);	

	return Plugin_Stop;
}

LoadMaps()
{
	new bool:fileFound;

	decl String:mapPath[256], String:mapFile[64];
	GetConVarString(g_Cvar_Mapfile, mapFile, 64);
	BuildPath(Path_SM, mapPath, sizeof(mapPath), mapFile);
	fileFound = FileExists(mapPath);
	if (!fileFound)
	{
		new Handle:mapCycleFile = FindConVar("mapcyclefile");
		GetConVarString(mapCycleFile, mapPath, sizeof(mapPath));
		fileFound = FileExists(mapPath);
	}
	
	if (!fileFound)
	{
		LogError("Unable to locate sm_randomcycle_file or mapcyclefile, no maps loaded.");
		
		if (g_MapList != INVALID_HANDLE)
		{
			ClearArray(g_MapList);
		}
		
		return 0;		
	}

	// If the file hasn't changed, there's no reason to reload
	// all of the maps.
	new fileTime =  GetFileTime(mapPath, FileTime_LastChange);
	if (g_mapFileTime == fileTime)
	{
		return GetArraySize(g_MapList);
	}
	
	g_mapFileTime = fileTime;
	
	// Reset the array
	if (g_MapList != INVALID_HANDLE)
	{
		ClearArray(g_MapList);
	}

	LogMessage("[SM] Loading Random Cycle map file [%s]", mapPath);

	new Handle:file = OpenFile(mapPath, "rt");
	if (file == INVALID_HANDLE)
	{
		LogError("[SM] Could not open file: %s", mapPath);
		return 0;
	}

	decl String:currentMap[32];
	GetCurrentMap(currentMap, sizeof(currentMap));
	
	decl String:buffer[64], len;
	while (!IsEndOfFile(file) && ReadFileLine(file, buffer, sizeof(buffer)))
	{
		TrimString(buffer);

		if ((len = StrContains(buffer, ".bsp", false)) != -1)
		{
			buffer[len] = '\0';
		}

		if (buffer[0] == '\0' || !IsValidConVarChar(buffer[0]) || !IsMapValid(buffer)
			|| strcmp(currentMap, buffer, false) == 0)
		{
			continue;
		}

		PushArrayString(g_MapList, buffer);
	}

	CloseHandle(file);
	return GetArraySize(g_MapList);
}