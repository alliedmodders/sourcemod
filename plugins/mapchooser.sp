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

public Plugin:myinfo =
{
	name = "MapChooser",
	author = "AlliedModders LLC",
	description = "Automated Map Voting",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_Cvar_Winlimit = INVALID_HANDLE;
new Handle:g_Cvar_Maxrounds = INVALID_HANDLE;
new Handle:g_Cvar_Fraglimit = INVALID_HANDLE;

new Handle:g_Cvar_Nextmap = INVALID_HANDLE;

new Handle:g_Cvar_StartTime = INVALID_HANDLE;
new Handle:g_Cvar_StartRounds = INVALID_HANDLE;
new Handle:g_Cvar_StartFrags = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeStep = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundStep = INVALID_HANDLE;
new Handle:g_Cvar_ExtendFragMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendFragStep = INVALID_HANDLE;
new Handle:g_Cvar_Mapfile = INVALID_HANDLE;
new Handle:g_Cvar_ExcludeMaps = INVALID_HANDLE;
new Handle:g_Cvar_IncludeMaps = INVALID_HANDLE;
new Handle:g_Cvar_NoVoteMode = INVALID_HANDLE;
new Handle:g_Cvar_Extend = INVALID_HANDLE;

new Handle:g_VoteTimer = INVALID_HANDLE;
new Handle:g_RetryTimer = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_OldMapList = INVALID_HANDLE;
new Handle:g_NextMapList = INVALID_HANDLE;
new Handle:g_VoteMenu = INVALID_HANDLE;
new Handle:g_TeamScores = INVALID_HANDLE;

new bool:g_HasVoteStarted;
new g_mapFileTime;

#define VOTE_EXTEND "##extend##"

public OnPluginStart()
{
	LoadTranslations("mapchooser.phrases");
	
	g_MapList = CreateArray(33);
	g_OldMapList = CreateArray(33);
	g_NextMapList = CreateArray(33);
	g_TeamScores = CreateArray(2);

	g_Cvar_StartTime = CreateConVar("sm_mapvote_start", "3.0", "Specifies when to start the vote based on time remaining.", _, true, 1.0);
	g_Cvar_StartRounds = CreateConVar("sm_mapvote_startround", "2.0", "Specifies when to start the vote based on rounds remaining.", _, true, 1.0);
	g_Cvar_StartFrags = CreateConVar("sm_mapvote_startfrags", "5.0", "Specifies when to start the vote base on frags remaining.", _, true, 1.0);
	g_Cvar_ExtendTimeMax = CreateConVar("sm_extendmap_maxtime", "90", "Specifies the maximum amount of time a map can be extended", _, true, 0.0);
	g_Cvar_ExtendTimeStep = CreateConVar("sm_extendmap_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundMax = CreateConVar("sm_extendmap_maxrounds", "30", "Specfies the maximum amount of rounds a map can be extended", _, true, 0.0);
	g_Cvar_ExtendRoundStep = CreateConVar("sm_extendmap_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 5.0);
	g_Cvar_ExtendFragMax = CreateConVar("sm_extendmap_maxfrags", "150", "Specfies the maximum frags allowed that a map can be extended.", _, true, 0.0);
	g_Cvar_ExtendFragStep = CreateConVar("sm_extendmap_fragstep", "10", "Specifies how many more frags are allowed when map is extended.", _, true, 5.0);	
	g_Cvar_Mapfile = CreateConVar("sm_mapvote_file", "configs/maps.ini", "Map file to use. (Def sourcemod/configs/maps.ini)");
	g_Cvar_ExcludeMaps = CreateConVar("sm_mapvote_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	g_Cvar_IncludeMaps = CreateConVar("sm_mapvote_include", "5", "Specifies how many maps to include in the vote.", _, true, 2.0, true, 6.0);
	g_Cvar_NoVoteMode = CreateConVar("sm_mapvote_novote", "1", "Specifies whether or not MapChooser should pick a map if no votes are received.", _, true, 0.0, true, 1.0);
	g_Cvar_Extend = CreateConVar("sm_mapvote_extend", "1", "Specifies whether or not MapChooser will allow the map to be extended.", _, true, 0.0, true, 1.0);

	RegAdminCmd("sm_mapvote", Command_Mapvote, ADMFLAG_CHANGEMAP, "sm_mapvote - Forces MapChooser to attempt to run a map vote now.");

	g_Cvar_Winlimit = FindConVar("mp_winlimit");
	g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	g_Cvar_Fraglimit = FindConVar("mp_fraglimit");
	
	if (g_Cvar_Winlimit != INVALID_HANDLE || g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		HookEvent("round_end", Event_RoundEnd);
	}
	
	if (g_Cvar_Fraglimit != INVALID_HANDLE)
	{
		HookEvent("player_death", Event_PlayerDeath);		
	}
	
	AutoExecConfig(true, "mapchooser");
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
		SetConVarString(g_Cvar_Nextmap, "Pending Vote");
	}
}

public OnMapEnd()
{
	g_HasVoteStarted = false;
	g_RetryTimer = INVALID_HANDLE;
	g_VoteTimer = INVALID_HANDLE;
}

public OnMapTimeLeftChanged()
{
	if (GetArraySize(g_MapList))
	{
		SetupTimeleftTimer();
	}
}

SetupTimeleftTimer()
{
	new time;
	if (GetMapTimeLeft(time) && time > 0)
	{
		new startTime = GetConVarInt(g_Cvar_StartTime) * 60;
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
	if (!GetArraySize(g_MapList))
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

/* You ask, why don't you just use team_score event? And I answer... Because CSS doesn't. */
public Event_RoundEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (!GetArraySize(g_MapList) || g_HasVoteStarted)
	{
		return;
	}

	new winner = GetEventInt(event, "winner");
	
	if (winner == 0 || winner == 1)
	{
		return;
	}

	static total;
	total++;
	
	new team[2], teamPos = -1;
	for (new i; i < GetArraySize(g_TeamScores); i++)
	{
		GetArrayArray(g_TeamScores, i, team);
		if (team[0] == winner)
		{
			teamPos = i;
			break;
		}		
	}
	
	if (teamPos == -1)
	{
		team[0] = winner;
		team[1] = 1;
		PushArrayArray(g_TeamScores, team);
	}
	else
	{
		team[1]++;
		SetArrayArray(g_TeamScores, teamPos, team);
	}	
	
	if (g_Cvar_Winlimit != INVALID_HANDLE)
	{
		new winlimit = GetConVarInt(g_Cvar_Winlimit);
		if (winlimit)
		{
			if (team[1] >= (winlimit - GetConVarInt(g_Cvar_StartRounds)))
			{
				InitiateVote();
			}
		}
	}
	
	if (g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
		if (maxrounds)
		{
			if (total >= (maxrounds - GetConVarInt(g_Cvar_StartRounds)))
			{
				InitiateVote();
			}			
		}
	}
}

public Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (!GetArraySize(g_MapList) || g_Cvar_Fraglimit == INVALID_HANDLE || g_HasVoteStarted)
	{
		return;
	}
	
	if (!GetConVarInt(g_Cvar_Fraglimit))
	{
		return;
	}

	new fragger = GetClientOfUserId(GetEventInt(event, "attacker"));
	if (fragger && GetClientFrags(fragger) >= (GetConVarInt(g_Cvar_Fraglimit) - GetConVarInt(g_Cvar_StartFrags)))
	{
		InitiateVote();
	}
}

public Action:Command_Mapvote(client, args)
{
	InitiateVote();

	return Plugin_Handled;	
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

	decl String:map[32];
	for (new i = 0; i < GetArraySize(g_NextMapList); i++)
	{
		GetArrayString(g_NextMapList, i, map, sizeof(map));
		AddMenuItem(g_VoteMenu, map, map);
	}

	if (GetConVarBool(g_Cvar_Extend))
	{
		new bool:allowExtend, time;
		if (GetMapTimeLimit(time) && time > 0 && time < GetConVarInt(g_Cvar_ExtendTimeMax))
		{
			allowExtend = true;
		}

		if (g_Cvar_Winlimit != INVALID_HANDLE && GetConVarInt(g_Cvar_Winlimit) < GetConVarInt(g_Cvar_ExtendRoundMax))
		{
			allowExtend = true;
		}	
	
		if (g_Cvar_Maxrounds != INVALID_HANDLE && GetConVarInt(g_Cvar_Maxrounds) < GetConVarInt(g_Cvar_ExtendRoundMax))
		{
			allowExtend = true;
		}
	
		if (g_Cvar_Fraglimit != INVALID_HANDLE && GetConVarInt(g_Cvar_Fraglimit) < GetConVarInt(g_Cvar_ExtendFragMax))
		{
			allowExtend = true;
		}

		if (allowExtend)
		{
			AddMenuItem(g_VoteMenu, VOTE_EXTEND, "Extend Map");
		}
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
		
		case MenuAction_DisplayItem:
		{
			if (GetMenuItemCount(menu) - 1 == param2)
			{
				decl String:map[64], String:buffer[255];
				GetMenuItem(menu, param2, map, sizeof(map));
				if (strcmp(map, VOTE_EXTEND, false) == 0)
				{
					Format(buffer, sizeof(buffer), "%T", "Extend Map", param1);
					return RedrawMenuItem(buffer);
				}
			}
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
			if (param1 == VoteCancel_NoVotes && GetConVarBool(g_Cvar_NoVoteMode))
			{
				new count = GetMenuItemCount(menu);
				new item = GetRandomInt(0, count - 1);
				decl String:map[32];
				GetMenuItem(menu, item, map, sizeof(map));
				
				while (strcmp(map, VOTE_EXTEND, false) == 0)
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

			if (strcmp(map, VOTE_EXTEND, false) == 0)
			{
				new time;
				if (GetMapTimeLimit(time))
				{
					if (time > 0 && time < GetConVarInt(g_Cvar_ExtendTimeMax))
					{
						ExtendMapTimeLimit(GetConVarInt(g_Cvar_ExtendTimeStep)*60);						
					}
				}
				
				if (g_Cvar_Winlimit != INVALID_HANDLE)
				{
					new winlimit = GetConVarInt(g_Cvar_Winlimit);
					if (winlimit && winlimit < GetConVarInt(g_Cvar_ExtendRoundMax))
					{
						SetConVarInt(g_Cvar_Winlimit, winlimit + GetConVarInt(g_Cvar_ExtendRoundStep));
					}					
				}
				
				if (g_Cvar_Maxrounds != INVALID_HANDLE)
				{
					new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
					if (maxrounds && maxrounds < GetConVarInt(g_Cvar_ExtendRoundMax))
					{
						SetConVarInt(g_Cvar_Maxrounds, maxrounds + GetConVarInt(g_Cvar_ExtendRoundStep));
					}
				}
				
				if (g_Cvar_Fraglimit != INVALID_HANDLE)
				{
					new fraglimit = GetConVarInt(g_Cvar_Fraglimit);
					if (fraglimit && fraglimit < GetConVarInt(g_Cvar_ExtendFragMax))
					{
						SetConVarInt(g_Cvar_Fraglimit, fraglimit + GetConVarInt(g_Cvar_ExtendFragStep));						
					}
				}

				PrintToChatAll("[SM] %t", "Current Map Extended");
				LogMessage("Voting for next map has finished. The current map has been extended.");
				
				// We extended, so we'll have to vote again.
				g_HasVoteStarted = false;
				CreateNextVote();
				SetupTimeleftTimer();
			}
			else
			{
				SetNextMap(map);
			}
		}
	}
	
	return 0;
}

SetNextMap(const String:map[])
{
	SetConVarString(g_Cvar_Nextmap, map);
	PushArrayString(g_OldMapList, map);
				
	if (GetArraySize(g_OldMapList) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		RemoveFromArray(g_OldMapList, 0);
	}
		
	PrintToChatAll("[SM] %t", "Nextmap Voting Finished", map);
	LogMessage("Voting for next map has finished. Nextmap: %s.", map);	
}

CreateNextVote()
{
	if(g_NextMapList != INVALID_HANDLE)
	{
		ClearArray(g_NextMapList);
	}
	
	new Handle:tempMaps  = CreateArray(33);
	decl String:map[32];
	for (new i = 0; i < GetArraySize(g_MapList); i++)
	{
		GetArrayString(g_MapList, i, map, sizeof(map));
		PushArrayString(tempMaps, map);		
	}
	
	if (GetArraySize(tempMaps) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		for (new i = 0; i < GetArraySize(g_OldMapList); i++)
		{
			GetArrayString(g_OldMapList, i, map, sizeof(map));
			for (new j = 0; j < GetArraySize(tempMaps); j++)
			{
				decl String:temp[32];
				GetArrayString(tempMaps, j, temp, sizeof(temp));
				if (strcmp(temp, map) == 0)
				{
					RemoveFromArray(tempMaps, j);
					break;
				}				
			}
		}	
	}

	new limit = (GetConVarInt(g_Cvar_IncludeMaps) < GetArraySize(tempMaps) ? GetConVarInt(g_Cvar_IncludeMaps) : GetArraySize(tempMaps));
	for (new i = 0; i < limit; i++)
	{
		new b = GetRandomInt(0, GetArraySize(tempMaps) - 1);
		GetArrayString(tempMaps, b, map, sizeof(map));		
		PushArrayString(g_NextMapList, map);
		RemoveFromArray(tempMaps, b);
	}
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
		LogError("Unable to locate sm_mapvote_file or mapcyclefile, no maps loaded.");
		
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

	LogMessage("[SM] Loading mapchooser map file [%s]", mapPath);

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