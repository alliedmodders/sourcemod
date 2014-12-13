/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Nextmap Plugin
 * Adds sm_nextmap cvar for changing map and nextmap chat trigger.
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

#include <sourcemod>
#include "include/nextmap.inc"

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo = 
{
	name = "Nextmap",
	author = "AlliedModders LLC",
	description = "Provides nextmap and sm_nextmap",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

int g_MapPos = -1;
Handle g_MapList = null;
int g_MapListSerial = -1;

int g_CurrentMapStartTime;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	char game[128];
	GetGameFolderName(game, sizeof(game));

	if (StrEqual(game, "left4dead", false)
			|| StrEqual(game, "dystopia", false)
			|| StrEqual(game, "synergy", false)
			|| StrEqual(game, "left4dead2", false)
			|| StrEqual(game, "garrysmod", false)
			|| StrEqual(game, "swarm", false)
			|| StrEqual(game, "dota", false))
	{
		strcopy(error, err_max, "Nextmap is incompatible with this game");
		return APLRes_SilentFailure;
	}

	return APLRes_Success;
}


public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("nextmap.phrases");
	
	g_MapList = CreateArray(32);

	RegAdminCmd("sm_maphistory", Command_MapHistory, ADMFLAG_CHANGEMAP, "Shows the most recent maps played");
	RegConsoleCmd("listmaps", Command_List);

	// Set to the current map so OnMapStart() will know what to do
	char currentMap[64];
	GetCurrentMap(currentMap, 64);
	SetNextMap(currentMap);
}

public void OnMapStart()
{
	g_CurrentMapStartTime = GetTime();
}
 
public void OnConfigsExecuted()
{
	char lastMap[64], currentMap[64];
	GetNextMap(lastMap, sizeof(lastMap));
	GetCurrentMap(currentMap, 64);
	
	// Why am I doing this? If we switched to a new map, but it wasn't what we expected (Due to sm_map, sm_votemap, or
	// some other plugin/command), we don't want to scramble the map cycle. Or for example, admin switches to a custom map
	// not in mapcyclefile. So we keep it set to the last expected nextmap. - ferret
	if (strcmp(lastMap, currentMap) == 0)
	{
		FindAndSetNextMap();
	}
}

public Action Command_List(int client, int args) 
{
	PrintToConsole(client, "Map Cycle:");
	
	int mapCount = GetArraySize(g_MapList);
	char mapName[32];
	for (int i = 0; i < mapCount; i++)
	{
		GetArrayString(g_MapList, i, mapName, sizeof(mapName));
		PrintToConsole(client, "%s", mapName);
	}
 
	return Plugin_Handled;
}
  
void FindAndSetNextMap()
{
	if (ReadMapList(g_MapList, 
			g_MapListSerial, 
			"mapcyclefile", 
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT)
		== null)
	{
		if (g_MapListSerial == -1)
		{
			LogError("FATAL: Cannot load map cycle. Nextmap not loaded.");
			SetFailState("Mapcycle Not Found");
		}
	}
	
	int mapCount = GetArraySize(g_MapList);
	char mapName[32];
	
	if (g_MapPos == -1)
	{
		char current[64];
		GetCurrentMap(current, 64);

		for (int i = 0; i < mapCount; i++)
		{
			GetArrayString(g_MapList, i, mapName, sizeof(mapName));
			if (strcmp(current, mapName, false) == 0)
			{
				g_MapPos = i;
				break;
			}
		}
		
		if (g_MapPos == -1)
			g_MapPos = 0;
	}
	
	g_MapPos++;
	if (g_MapPos >= mapCount)
		g_MapPos = 0;	
 
 	GetArrayString(g_MapList, g_MapPos, mapName, sizeof(mapName));
	SetNextMap(mapName);
}

public Action Command_MapHistory(int client, int args)
{
	int mapCount = GetMapHistorySize();
	
	char mapName[32];
	char changeReason[100];
	char timeString[100];
	char playedTime[100];
	int startTime;
	
	int lastMapStartTime = g_CurrentMapStartTime;
	
	PrintToConsole(client, "Map History:\n");
	PrintToConsole(client, "Map : Started : Played Time : Reason for ending");
	
	GetCurrentMap(mapName, sizeof(mapName));
	PrintToConsole(client, "%02i. %s (Current Map)", 0, mapName);
	
	for (int i=0; i<mapCount; i++)
	{
		GetMapHistory(i, mapName, sizeof(mapName), changeReason, sizeof(changeReason), startTime);

		FormatTimeDuration(timeString, sizeof(timeString), GetTime() - startTime);
		FormatTimeDuration(playedTime, sizeof(playedTime), lastMapStartTime - startTime);
		
		PrintToConsole(client, "%02i. %s : %s ago : %s : %s", i+1, mapName, timeString, playedTime, changeReason);
		
		lastMapStartTime = startTime;
	}

	return Plugin_Handled;
}

int FormatTimeDuration(char[] buffer, int maxlen, int time)
{
	int days = time / 86400;
	int hours = (time / 3600) % 24;
	int minutes = (time / 60) % 60;
	int seconds =  time % 60;
	
	if (days > 0)
	{
		return Format(buffer, maxlen, "%id %ih %im", days, hours, (seconds >= 30) ? minutes+1 : minutes);
	}
	else if (hours > 0)
	{
		return Format(buffer, maxlen, "%ih %im", hours, (seconds >= 30) ? minutes+1 : minutes);		
	}
	else if (minutes > 0)
	{
		return Format(buffer, maxlen, "%im", (seconds >= 30) ? minutes+1 : minutes);		
	}
	else
	{
		return Format(buffer, maxlen, "%is", seconds);		
	}
}
