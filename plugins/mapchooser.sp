/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Mapchooser Plugin
 * Creates a map vote at appropriate times, setting sm_nextmap to the winning
 * vote
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

#define MAX_MAPS_SELECTION		5

new Handle:g_Cvar_Winlimit = INVALID_HANDLE;
new Handle:g_Cvar_Maxrounds = INVALID_HANDLE;

new Handle:g_Cvar_Nextmap = INVALID_HANDLE;
new Handle:g_Cvar_StartTime = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeStep = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundsMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundsStep = INVALID_HANDLE;
new Handle:g_Cvar_Mapfile = INVALID_HANDLE;
new Handle:g_Cvar_ExcludeMaps = INVALID_HANDLE;

new Handle:g_VoteTimer = INVALID_HANDLE;
new Handle:g_RetryTimer = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_OldMapList = INVALID_HANDLE;
new Handle:g_NextMapList = INVALID_HANDLE;
new Handle:g_VoteMenu = INVALID_HANDLE;

new g_MapCount;
new g_GameType;
new bool:g_HasVoteStarted;
new g_mapFileTime;

enum
{
	GAME_HL2 = 0,
	GAME_CSS,
	GAME_DOD
}

public OnPluginStart()
{
	LoadTranslations("mapchooser.phrases");
	
	g_MapList = CreateArray(33);
	g_OldMapList = CreateArray(33);

	g_Cvar_StartTime = CreateConVar("sm_mapvote_start", "3.0", "Specifies the time to start the vote when this much time remains.", _, true, 1.0);
	g_Cvar_ExtendTimeMax = CreateConVar("sm_extendmap_maxtime", "90", "Specifies the maximum amount of time a map can be extended");
	g_Cvar_ExtendTimeStep = CreateConVar("sm_extendmap_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundsMax = CreateConVar("sm_extendmap_maxrounds", "30", "Specfies the maximum amount of rounds a map can be extended");
	g_Cvar_ExtendRoundsStep = CreateConVar("sm_extendmap_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 5.0);
	g_Cvar_Mapfile = CreateConVar("sm_mapvote_file", "configs/maps.ini", "Map file to use. (Def sourcemod/configs/maps.ini)");
	g_Cvar_ExcludeMaps = CreateConVar("sm_mapvote_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);

	decl String:FolderName[32];
	GetGameFolderName(FolderName, sizeof(FolderName));

	if (strcmp(FolderName, "cstrike", false) == 0)
	{
		g_GameType = GAME_CSS;
		g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	}
	else if (strcmp(FolderName, "dod", false) == 0)
	{
		g_GameType = GAME_DOD;	
	}
	
	if (g_GameType)
	{
		g_Cvar_Winlimit = FindConVar("mp_winlimit");
		HookEvent("team_score", Event_TeamScore);
	}
}

public OnMapStart()
{
	g_Cvar_Nextmap = FindConVar("sm_nextmap");

	if (g_Cvar_Nextmap == INVALID_HANDLE)
	{
		LogError("FATAL: Cannot find sm_nextmap cvar. Mapchooser not loaded.");
		SetFailState("sm_nextmap not found");
	}
	
	if (LoadMaps())
	{
		CreateNextVote();
		SetupTimeleftTimer();
	}
}

public OnMapEnd()
{
	g_HasVoteStarted = false;
}

public OnMapTimeLeftChanged()
{
	SetupTimeleftTimer();
}

SetupTimeleftTimer()
{
	new time;
	if (GetMapTimeLeft(time) && time > 0)
	{
		new startTime = GetConVarInt(g_Cvar_StartTime);
		if (time - startTime < 0)
		{
			InitiateVote();			
		}
		else
		{
			if (g_VoteTimer != INVALID_HANDLE)
			{
				KillTimer(g_VoteTimer);
				g_VoteTimer = INVALID_HANDLE;
			}			
			
			g_VoteTimer = CreateTimer(float(time - startTime), Timer_StartMapVote, TIMER_FLAG_NO_MAPCHANGE);
		}		
	}
}

public Action:Timer_StartMapVote(Handle:timer)
{
	if (!g_MapCount)
	{
		return Plugin_Stop;
	}	
	
	if (timer == g_RetryTimer)
	{
		g_RetryTimer = INVALID_HANDLE;
	}
	else
	{
		g_VoteTimer = INVALID_HANDLE;
	}

	InitiateVote();

	return Plugin_Stop;
}

public Event_TeamScore(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (!g_MapCount)
	{
		return;
	}

	static Score[2];
	new Team = GetEventInt(event, "teamid");
	Score[Team - 2] = GetEventInt(event, "score");

	new winlimit = GetConVarInt(g_Cvar_Winlimit);
	if (winlimit)
	{
		new Limit = winlimit - 2;
		if (Score[0] > Limit || Score[1] > Limit)
		{
			InitiateVote();
		}
	}
	else if (g_GameType == GAME_CSS)
	{
		new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
		if (maxrounds)
		{
			if ((Score[0] + Score[1]) > maxrounds - 2)
			{
				InitiateVote();
			}			
		}
	}
}

InitiateVote()
{
	if (g_HasVoteStarted || g_RetryTimer != INVALID_HANDLE)
	{
		return;
	}
	
	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		g_RetryTimer = CreateTimer(5.0, Timer_StartMapVote, TIMER_FLAG_NO_MAPCHANGE);
		return;
	}		
	
	g_HasVoteStarted = true;
	g_VoteMenu = CreateMenu(Handler_MapVoteMenu, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(g_VoteMenu, "Vote Nextmap");

	KvRewind(g_NextMapList);
	KvGotoFirstSubKey(g_NextMapList);

	decl String:map[32];
	do
	{
		KvGetSectionName(g_NextMapList, map, sizeof(map));
		AddMenuItem(g_VoteMenu, map, map);
	}
	while (KvGotoNextKey(g_NextMapList));

	new bool:AllowExtend, time;
	if (GetMapTimeLimit(time) && time > 0 && time < GetConVarInt(g_Cvar_ExtendTimeMax))
	{
		AllowExtend = true;
	}
	
	if (g_GameType)
	{
		// Yes, I could short circuit this above. But I find it cleaner to break
		// it up into two if's
		if (GetConVarInt(g_Cvar_Maxrounds) < GetConVarInt(g_Cvar_ExtendRoundsMax)
			|| GetConVarInt(g_Cvar_Winlimit) < GetConVarInt(g_Cvar_ExtendRoundsMax))
		{
			AllowExtend = true;
		}
	}	

	if (AllowExtend)
	{
		AddMenuItem(g_VoteMenu, "##extend##", "Extend");
	}

	SetMenuExitButton(g_VoteMenu, false);
	VoteMenuToAll(g_VoteMenu, 20);

	LogMessage("Voting for next map has started.");
	PrintToChatAll("[SM] %t", "Nextmap Voting Started");
}

public Handler_MapVoteMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = INVALID_HANDLE;
			CloseHandle(menu);
		}
		
		case MenuAction_Display:
		{
	 		decl String:buffer[255];
			Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);

			new Handle:panel = Handle:param2;
			SetPanelTitle(panel, buffer);
		}

		// Why am I commented out? Because BAIL hasn't decided yet if
		// vote notification will be built into the Vote API.
		/*case MenuAction_Select:
		{
			decl String:Name[32], String:Map[32];
			GetClientName(param1, Name, sizeof(Name));
			GetMenuItem(menu, param2, Map, sizeof(Map));

			PrintToChatAll("[SM] %s has voted for map '%s'", Name, Map);
		}*/
		
		case MenuAction_VoteCancel:
		{
			// If we receive 0 votes, pick at random.
			if (param1 == VoteCancel_NoVotes)
			{
				new count = GetMenuItemCount(menu);
				new item = GetRandomInt(0, count - 1);
				decl String:map[32];
				GetMenuItem(menu, item, map, sizeof(map));
				
				while (strcmp(map, "##extend##", false) == 0)
				{
					item = GetRandomInt(0, count - 1);
					GetMenuItem(menu, item, map, sizeof(map));
				}
				
				SetNextMap(map);			
			}
			else
			{
				// We were actually cancelled. What should we do?
			}
		}

		case MenuAction_VoteEnd:
		{
			decl String:map[32];
			GetMenuItem(menu, param1, map, sizeof(map));

			if (strcmp(map, "##extend##", false) == 0)
			{
				new time;
				if (GetMapTimeLimit(time))
				{
					if (time > 0 && time < GetConVarInt(g_Cvar_ExtendTimeMax))
					{
						ExtendMapTimeLimit(GetConVarInt(g_Cvar_ExtendTimeStep));						
					}
				}
				
				if (g_GameType)
				{
					new roundstep = GetConVarInt(g_Cvar_ExtendRoundsStep);
					new winlimit = GetConVarInt(g_Cvar_Winlimit);
					if (winlimit < GetConVarInt(g_Cvar_ExtendRoundsMax))
					{
						SetConVarInt(g_Cvar_Winlimit, winlimit + roundstep);
					}
					
					if (g_GameType == GAME_CSS)
					{
						new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
						if (maxrounds < GetConVarInt(g_Cvar_ExtendRoundsMax))
						{
							SetConVarInt(g_Cvar_Maxrounds, maxrounds + roundstep);
						}
					}
				}

				PrintToChatAll("[SM] %t", "Current Map Extended");
				LogMessage("Voting for next map has finished. The current map has been extended.");
				
				// We extended, so we have to vote again.
				g_HasVoteStarted = false;
			}
			else
			{
				SetNextMap(map);
			}
		}
	}
}

SetNextMap(const String:map[])
{
	SetConVarString(g_Cvar_Nextmap, map);
	PushArrayString(g_OldMapList, map);
				
	if (GetArraySize(g_OldMapList) > 5)
	{
		RemoveFromArray(g_OldMapList, 0);
	}
		
	PrintToChatAll("[SM] %t", "Nextmap Voting Finished");
	LogMessage("Voting for next map has finished. Nextmap: %s.", map);	
}


CreateNextVote()
{
	g_NextMapList = CreateKeyValues("MapChooser");
	
	new bool:oldMaps = false;
	if (g_MapCount > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		oldMaps = true;
	}
	
	decl String:map[32];
	for (new i = 0, b, count; i < g_MapCount; i++)
	{
		b = GetRandomInt(0, g_MapCount - 1);
		GetArrayString(g_MapList, b, map, sizeof(map));

		if (!IsMapSelected(map) && !(oldMaps && IsMapOld(map)))
		{
			
			KvJumpToKey(g_NextMapList, map, true);
			count++;
		}

		if (count == MAX_MAPS_SELECTION || count >= 9)
		{
			break;
		}
	}
}

LoadMaps()
{
	new bool:fileFound;

	decl String:mapPath[256], String:mapFile[64];
	GetConVarString(g_Cvar_Mapfile, mapFile, 64);
	BuildPath(Path_SM, mapPath, sizeof(mapFile), mapFile);
	fileFound = FileExists(mapPath);
	if (!fileFound)
	{
		new Handle:mapCycleFile = FindConVar("mapcyclefile");
		GetConVarString(mapCycleFile, mapPath, sizeof(mapPath));
		fileFound = FileExists(mapPath);
	}
	
	if (!fileFound)
	{
		LogError("Unable to locate g_Cvar_Mapfile or mapcyclefile, no maps loaded.");
		
		g_MapCount = 0;
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
		return g_MapCount;
	}
	
	g_mapFileTime = fileTime;
	
	// Reset the array
	g_MapCount = 0;
	if (g_MapList != INVALID_HANDLE)
	{
		ClearArray(g_MapList);
	}

	PrintToServer("[SM] Loading mapchooser map file [%s]", mapPath);

	decl String:currentMap[32];
	GetCurrentMap(currentMap, sizeof(currentMap));

	new Handle:file = OpenFile(mapPath, "rt");
	if (file == INVALID_HANDLE)
	{
		LogError("[SM] Could not open file: %s", mapPath);
		return 0;
	}

	decl String:buffer[64], len;
	while (!IsEndOfFile(file) && ReadFileLine(file, buffer, sizeof(buffer)))
	{
		TrimString(buffer);

		if ((len = StrContains(buffer, ".bsp", false)) != -1)
		{
			buffer[len] = '\0';
		}

		if (buffer[0] == '\0' || !IsValidCvarChar(buffer[0]) || !IsMapValid(buffer)
			|| strcmp(currentMap, buffer, false) == 0)
		{
			continue;
		}

		PushArrayString(g_MapList, buffer);
		g_MapCount++;
	}

	CloseHandle(file);
	return g_MapCount;
}

stock bool:IsValidCvarChar(c)
{
	return (c == '_' || IsCharAlpha(c) || IsCharNumeric(c));
}

stock bool:IsMapSelected(const String:Map[])
{
	KvRewind(g_NextMapList);
	if (KvJumpToKey(g_NextMapList, Map))
	{
		return true;
	}

	return false;
}

stock bool:IsMapOld(const String:map[])
{
	decl String:oldMap[64];
	
	for (new i = 0; i < GetArraySize(g_OldMapList); i++)
	{
		GetArrayString(g_OldMapList, i, oldMap, sizeof(oldMap));
		if(strcmp(map, oldMap, false) == 0)
		{
			return false;
		}		
	}

	return true;
}