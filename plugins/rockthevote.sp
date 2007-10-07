/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Rock The Vote Plugin
 * Creates a map vote when the required number of players have requested one.
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

#include <sourcemod>

#pragma semicolon 1

public Plugin:myinfo =
{
	name = "Rock The Vote",
	author = "AlliedModders LLC",
	description = "Provides RTV Map Voting",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_Cvar_Needed = INVALID_HANDLE;
new Handle:g_Cvar_File = INVALID_HANDLE;
new Handle:g_Cvar_Maps = INVALID_HANDLE;
new Handle:g_Cvar_Nominate = INVALID_HANDLE;
new Handle:g_Cvar_MinPlayers = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_RTVMapList = INVALID_HANDLE;
new Handle:g_MapMenu = INVALID_HANDLE;
new Handle:g_RetryTimer = INVALID_HANDLE;
new g_mapFileTime;

new bool:g_CanRTV = false;		// True if RTV loaded maps and is active.
new bool:g_RTVAllowed = false;	// True if RTV is available to players. Used to delay rtv votes.
new bool:g_RTVStarted = false;	// Indicates that the actual map vote has started
new bool:g_RTVEnded = false;	// Indicates that the actual map vote has concluded
new g_Voters = 0;				// Total voters connected. Doesn't include fake clients.
new g_Votes = 0;				// Total number of "say rtv" votes
new g_VotesNeeded = 0;			// Necessary votes before map vote begins. (voters * percent_needed)
new bool:g_Voted[MAXPLAYERS+1] = {false, ...};
new bool:g_Nominated[MAXPLAYERS+1] = {false, ...};

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("rockthevote.phrases");
	
	g_MapList = CreateArray(33);
	g_RTVMapList = CreateArray(33);
	
	g_Cvar_Needed = CreateConVar("sm_rtv_needed", "0.60", "Percentage of players needed to rockthevote (Def 60%)", 0, true, 0.05, true, 1.0);
	g_Cvar_File = CreateConVar("sm_rtv_file", "configs/maps.ini", "Map file to use. (Def configs/maps.ini)");
	g_Cvar_Maps = CreateConVar("sm_rtv_maps", "4", "Number of maps to be voted on. 2 to 6. (Def 4)", 0, true, 2.0, true, 6.0);
	g_Cvar_Nominate = CreateConVar("sm_rtv_nominate", "1", "Enables nomination system.", 0, true, 0.0, true, 1.0);
	g_Cvar_MinPlayers = CreateConVar("sm_rtv_minplayers", "0", "Number of players required before RTV will be enabled.", 0, true, 0.0, true, float(MAXPLAYERS));
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	RegAdminCmd("sm_rtv_addmap", Command_Addmap, ADMFLAG_CHANGEMAP, "sm_rtv_addmap <mapname> - Forces a map to be on the RTV, and lowers the allowed nominations.");
	
	AutoExecConfig(true, "rtv");
}

public OnMapStart()
{
	if (g_RTVMapList != INVALID_HANDLE)
	{
		ClearArray(g_RTVMapList);
	}
	
	g_Voters = 0;
	g_Votes = 0;
	g_VotesNeeded = 0;
	g_RTVStarted = false;
	g_RTVEnded = false;
	
	if (LoadMaps())
	{
		BuildMapMenu();
		g_CanRTV = true;
		CreateTimer(30.0, Timer_DelayRTV);
	}
	else
	{
		LogMessage("[SM] Cannot find map cycle file, RTV not active.");
	}
}

public OnMapEnd()
{
	g_CanRTV = false;	
	g_RTVAllowed = false;
}


public bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	if(IsFakeClient(client))
		return true;
	
	g_Voted[client] = false;
	g_Nominated[client] = false;

	g_Voters++;
	g_VotesNeeded = RoundToFloor(float(g_Voters) * GetConVarFloat(g_Cvar_Needed));
	
	return true;
}

public OnClientDisconnect(client)
{
	if(IsFakeClient(client))
		return;
	
	if(g_Voted[client])
	{
		g_Votes--;
	}
	
	g_Voters--;
	
	g_VotesNeeded = RoundToFloor(float(g_Voters) * GetConVarFloat(g_Cvar_Needed));
	
	if (g_Votes && g_Voters && g_Votes >= g_VotesNeeded && g_RTVAllowed) 
	{
		CreateTimer(2.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
	}	
}

public Action:Command_Addmap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_rtv_addmap <mapname>");
		return Plugin_Handled;
	}
	
	decl String:mapname[64];
	GetCmdArg(1, mapname, sizeof(mapname));

	if (!IsStringInArray(g_MapList, mapname))
	{
		ReplyToCommand(client, "%t", "Map was not found", mapname);
		return Plugin_Handled;
	}
	
	if (GetArraySize(g_RTVMapList) > 0)
	{
		for (new i = 0; i < GetArraySize(g_RTVMapList); i++)
		{
			decl String:nextmap[64];
			GetArrayString(g_RTVMapList, i, nextmap, sizeof(nextmap));
			if (strcmp(mapname, nextmap, false) == 0)
			{
				ReplyToCommand(client, "%t", "Map Already In Vote", mapname);
				return Plugin_Handled;			
			}		
		}
		
		ShiftArrayUp(g_RTVMapList, 0);
		SetArrayString(g_RTVMapList, 0, mapname);
		
		while (GetArraySize(g_RTVMapList) > GetConVarInt(g_Cvar_Maps))
		{
			RemoveFromArray(g_RTVMapList, GetConVarInt(g_Cvar_Maps));
		}
	}
	else
	{
		PushArrayString(g_RTVMapList, mapname);
	}
		
	decl String:item[64];
	for (new i = 0; i < GetMenuItemCount(g_MapMenu); i++)
	{
		GetMenuItem(g_MapMenu, i, item, sizeof(item));
		if (strcmp(item, mapname) == 0)
		{
			RemoveMenuItem(g_MapMenu, i);
			break;
		}			
	}	
	
	ReplyToCommand(client, "%t", "Map Inserted", mapname);
	LogAction(client, -1, "\"%L\" inserted map \"%s\".", client, mapname);

	return Plugin_Handled;		
}

public Action:Command_Say(client, args)
{
	if (!g_CanRTV || !client)
		return Plugin_Continue;

	decl String:text[192];
	if (!GetCmdArgString(text, sizeof(text)))
	{
		return Plugin_Continue;
	}
	
	new startidx = 0;
	if(text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}
	
	if (strcmp(text[startidx], "rtv", false) == 0 || strcmp(text[startidx], "rockthevote", false) == 0)
	{
		if (!g_RTVAllowed)
		{
			PrintToChat(client, "[SM] %t", "RTV Not Allowed");
			return Plugin_Continue;
		}
		
		if (g_RTVEnded)
		{
			PrintToChat(client, "[SM] %t", "RTV Ended");
			return Plugin_Continue;
		}
		
		if (g_RTVStarted)
		{
			PrintToChat(client, "[SM] %t", "RTV Started");
			return Plugin_Continue;
		}
		
		if (GetClientCount(true) < GetConVarInt(g_Cvar_MinPlayers) && g_Votes == 0) // Should we keep checking g_Votes here?
		{
			PrintToChat(client, "[SM] %t", "Minimal Players Not Met");
			return Plugin_Continue;			
		}
		
		if (g_Voted[client])
		{
			PrintToChat(client, "[SM] %t", "Already Voted");
			return Plugin_Continue;
		}	
		
		new String:name[64];
		GetClientName(client, name, sizeof(name));
		
		g_Votes++;
		g_Voted[client] = true;
		
		PrintToChatAll("[SM] %t", "RTV Requested", name, g_Votes, g_VotesNeeded);
		
		if (g_Votes >= g_VotesNeeded)
		{
			CreateTimer(2.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
		}
	}
	else if (GetConVarBool(g_Cvar_Nominate) && strcmp(text[startidx], "nominate", false) == 0)
	{
		if (g_RTVEnded)
		{
			PrintToChat(client, "[SM] %t", "RTV Ended");
			return Plugin_Continue;
		}		
		
		if (g_RTVStarted)
		{
			PrintToChat(client, "[SM] %t", "RTV Started");
			return Plugin_Continue;
		}
		
		if (g_Nominated[client])
		{
			PrintToChat(client, "[SM] %t", "Already Nominated");
			return Plugin_Continue;
		}
		
		if (GetArraySize(g_RTVMapList) >= GetConVarInt(g_Cvar_Maps))
		{
			PrintToChat(client, "[SM] %t", "Max Nominations");
			return Plugin_Continue;			
		}
		
		DisplayMenu(g_MapMenu, client, MENU_TIME_FOREVER);		
	}
	
	return Plugin_Continue;	
}

public Action:Timer_DelayRTV(Handle:timer)
{
	g_RTVAllowed = true;
}

public Action:Timer_StartRTV(Handle:timer)
{
	if (timer == g_RetryTimer)
	{
		g_RetryTimer = INVALID_HANDLE;
	}
	
	if (g_RetryTimer != INVALID_HANDLE)
	{
		return;
	}

	g_RTVStarted = true;

	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		g_RetryTimer = CreateTimer(5.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
		return;
	}			
	
	PrintToChatAll("[SM] %t", "RTV Vote Ready");
		
	new Handle:MapVoteMenu = CreateMenu(Handler_MapMapVoteMenu, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(MapVoteMenu, "Rock The Vote");
	
	new limit = (GetArraySize(g_RTVMapList) < GetConVarInt(g_Cvar_Maps) ? GetArraySize(g_RTVMapList) : GetConVarInt(g_Cvar_Maps));
	for (new i = 0; i < limit; i++)
	{
		decl String:rtvmap[64];
		GetArrayString(g_RTVMapList, i, rtvmap, sizeof(rtvmap));
		AddMenuItem(MapVoteMenu, rtvmap, rtvmap);
	}

	limit = (GetArraySize(g_MapList) < GetConVarInt(g_Cvar_Maps) ? GetArraySize(g_MapList) : GetConVarInt(g_Cvar_Maps));
	limit -= GetMenuItemCount(MapVoteMenu);
	
	if (limit < 0)
	{
		limit = 0;
	}
	
	for (new i = 0; i < limit; i++)
	{
		decl String:map[64];
		new b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
		GetArrayString(g_MapList, b, map, sizeof(map));
		
		while (IsStringInArray(g_RTVMapList, map))
		{
			b = GetRandomInt(0, GetArraySize(g_MapList) - 1);
			GetArrayString(g_MapList, b, map, sizeof(map));
		}
		
		PushArrayString(g_RTVMapList, map);
		AddMenuItem(MapVoteMenu, map, map);		
	}

	AddMenuItem(MapVoteMenu, "Don't Change", "Don't Change");
		
	SetMenuExitButton(MapVoteMenu, false);
	VoteMenuToAll(MapVoteMenu, 20);
		
	LogMessage("[SM] Rockthevote was successfully started.");
}

public Action:Timer_ChangeMap(Handle:hTimer, Handle:dp)
{
	new String:map[65];
	
	ResetPack(dp);
	ReadPackString(dp, map, sizeof(map));
	
	ServerCommand("changelevel \"%s\"", map);
	
	return Plugin_Stop;
}

public Handler_MapMapVoteMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			CloseHandle(menu);
		}
		
		case MenuAction_Display:
		{
	 		decl String:oldTitle[255], String:buffer[255];
			GetMenuTitle(menu, oldTitle, sizeof(oldTitle));
			Format(buffer, sizeof(buffer), "%T", oldTitle, param1);

			new Handle:panel = Handle:param2;
			SetPanelTitle(panel, buffer);
		}
		
		case MenuAction_DisplayItem:
		{
			if (GetMenuItemCount(menu) - 1 == param2)
			{
				decl String:buffer[255];
				Format(buffer, sizeof(buffer), "%T", "Don't Change", param1);
				return RedrawMenuItem(buffer);
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
			if (param1 == VoteCancel_NoVotes)
			{
				PrintToChatAll("[SM] %t", "No Votes");		
			}
		}

		case MenuAction_VoteEnd:
		{
			new String:map[64];
			
			GetMenuItem(menu, param1, map, sizeof(map));
			
			if (GetMenuItemCount(menu) - 1 == param1) // This should always match the "Keep Current" option
			{
				PrintToChatAll("[SM] %t", "Current Map Stays");
				LogMessage("[SM] Rockthevote has ended, current map kept.");
			}
			else
			{
				PrintToChatAll("[SM] %t", "Changing Maps", map);
				LogMessage("[SM] Rockthevote has ended, changing to map %s.", map);
				new Handle:dp;
				CreateDataTimer(5.0, Timer_ChangeMap, dp);
				WritePackString(dp, map);
			}
			
			g_RTVEnded = true;
		}
	}
	
	return 0;
}

public Handler_MapSelectMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			if (GetArraySize(g_RTVMapList) >= GetConVarInt(g_Cvar_Maps)) 
			{
				PrintToChat(param1, "[SM] %t", "Max Nominations");
				return;	
			}
			
			decl String:map[64], String:name[64];
			GetMenuItem(menu, param2, map, sizeof(map));
			
			if (IsStringInArray(g_RTVMapList, map))
			{
				PrintToChat(param1, "[SM] %t", "Map Already Nominated");
				return;
			}
			
			GetClientName(param1, name, 64);

			PushArrayString(g_RTVMapList, map);
			RemoveMenuItem(menu, param2);
			
			g_Nominated[param1] = true;
			
			PrintToChatAll("[SM] %t", "Map Nominated", name, map);
		}
	}
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

LoadMaps()
{
	new bool:fileFound;

	decl String:mapPath[256], String:mapFile[64];
	GetConVarString(g_Cvar_File, mapFile, 64);
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
		LogError("Unable to locate sm_rtv_file or mapcyclefile, no maps loaded.");
		
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

	LogMessage("[SM] Loading RTV map file [%s]", mapPath);

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