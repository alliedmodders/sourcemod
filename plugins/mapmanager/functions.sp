/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Management Plugin
 * Contains misc functions.
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

SetNextMap(String:map[])
{
	strcopy(g_NextMap, sizeof(g_NextMap), map);
	SetConVarString(g_Cvar_Nextmap, map);
}

SetMapChange(client, map, when, Float:time = 3.0)
{
	g_MapChangeSet = true;
	g_MapChangeWhen = when[0];
	SetNextMap(map);
 
	if (when[0] == 'r')
	{
 		ShowActivity2(client, "[SM] ", "%t", "Changing map end of round", map);
 		LogAction(client, -1, "\"%L\" set end of round map change to \"%s\"", client, map);
	}
	else if (when[0] == 'e')
	{
		ShowActivity2(client, "[SM] ", "%t", "Set nextmap", map);
		LogAction(client, -1, "\"%L\" set the next map to \"%s\"", client, map);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Change map", map);
		LogAction(client, -1, "\"%L\" changed map to \"%s\"", client, map);
		
		g_MapChanged = true;
	 	CreateTimer(3.0, Timer_ChangeMap);
	}
}

FindAndSetNextMap()
{
	if (ReadMapList(g_MapList, 
			g_MapListSerial, 
			"mapcyclefile", 
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT)
		== INVALID_HANDLE)
	{
		if (g_MapListSerial == -1)
		{
			LogError("FATAL: Cannot load map cycle. Nextmap not loaded.");
			SetFailState("Mapcycle Not Found");
		}
	}
	
	new mapCount = GetArraySize(g_MapList);
	decl String:mapName[32];
	
	if (g_MapPos == -1)
	{
		decl String:current[64];
		GetCurrentMap(current, 64);

		for (new i = 0; i < mapCount; i++)
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
	SetConVarString(g_Cvar_Nextmap, mapName);
}

Float:GetVotePercent(votes, totalVotes)
{
	return FloatDiv(float(votes),float(totalVotes));
}

bool:TestVoteDelay(client)
{
 	new delay = CheckVoteDelay();
 	
 	if (delay > 0)
 	{
 		if (delay > 60)
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Minutes", delay % 60);
 		}
 		else
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Seconds", delay);
 		}
 		
 		return false;
 	}
 	
	return true;
}

BuildMapMenu()
{
	if (g_MapMenu != INVALID_HANDLE)
	{
		CloseHandle(g_MapMenu);
		g_MapMenu = INVALID_HANDLE;
	}
	
	g_MapMenu = CreateMenu(Handler_MapSelectMenu);
	SetMenuTitle(g_MapMenu, "%t", "Nominate Title");

	decl String:map[64];		
	for (new i = 0; i < GetArraySize(g_MapList); i++)
	{
		GetArrayString(g_MapList, i, map, sizeof(map));
		AddMenuItem(g_MapMenu, map, map);
	}
	
	SetMenuExitButton(g_MapMenu, false);
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
			
			g_VoteTimer = CreateTimer(float(time - startTime), Timer_StartMapVote);
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
		g_RetryTimer = CreateTimer(5.0, Timer_StartMapVote);
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
	
	decl String:map[32];
	new index, Handle:tempMaps  = CloneArray(g_MapList);
	
	GetCurrentMap(map, sizeof(map));
	index = FindStringInArray(tempMaps, map);
	if (index != -1)
	{
		RemoveFromArray(tempMaps, index);
	}	
	
	if (GetConVarInt(g_Cvar_ExcludeMaps) && GetArraySize(tempMaps) > GetConVarInt(g_Cvar_ExcludeMaps))
	{
		for (new i = 0; i < GetArraySize(g_OldMapList); i++)
		{
			GetArrayString(g_OldMapList, i, map, sizeof(map));
			index = FindStringInArray(tempMaps, map);
			if (index != -1)
			{
				RemoveFromArray(tempMaps, index);
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
	
	CloseHandle(tempMaps);
}

/*

new Handle:g_map_array = INVALID_HANDLE;
new g_map_serial = -1;

LoadMapList(Handle:menu)
{
	new Handle:map_array;
	
	if ((map_array = ReadMapList(g_map_array,
			g_map_serial,
			"sm_map menu",
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER))
		!= INVALID_HANDLE)
	{
		g_map_array = map_array;
	}
	
	if (g_map_array == INVALID_HANDLE)
	{
		return 0;
	}
	
	RemoveAllMenuItems(menu);
	
	decl String:map_name[64];
	new map_count = GetArraySize(g_map_array);
	
	for (new i = 0; i < map_count; i++)
	{
		GetArrayString(g_map_array, i, map_name, sizeof(map_name));
		AddMenuItem(menu, map_name, map_name);
	}
	
	return map_count;
}

*/