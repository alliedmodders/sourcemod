/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains callbacks for commands
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
 
 public Action:Command_Say(client, args)
 {
	decl String:text[192];
	if (GetCmdArgString(text, sizeof(text)) < 1)
	{
		return Plugin_Continue;
	}
	
	new startidx;
	if (text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}
	
	decl String:message[8];
	BreakString(text[startidx], message, sizeof(message));
	
	if (strcmp(message, "nextmap", false) == 0)
	{
		decl String:map[32];
		GetConVarString(g_Cvar_Nextmap, map, sizeof(map));
		
		PrintToChat(client, "%t", "Next Map", map);
	}
	else
	{
		if (GetConVarBool(g_Cvar_RockTheVote) && (strcmp(text[startidx], "rtv", false) == 0 || strcmp(text[startidx], "rockthevote", false) == 0))
		{
			if (g_MapChanged)
			{
				ReplyToCommand(client, "[SM] %t", "Map change in progress");
				return Plugin_Continue;
			}			
			
			if (!g_RTV_Allowed)
			{
				PrintToChat(client, "[SM] %t", "RTV Not Allowed");
				return Plugin_Continue;
			}
			
			if (g_RTV_Ended)
			{
				PrintToChat(client, "[SM] %t", "RTV Ended");
				return Plugin_Continue;
			}
			
			if (g_RTV_Started)
			{
				PrintToChat(client, "[SM] %t", "RTV Started");
				return Plugin_Continue;
			}
			
			if (GetClientCount(true) < GetConVarInt(g_Cvar_MinPlayers) && g_RTV_Votes == 0) // Should we keep checking g_Votes here?
			{
				PrintToChat(client, "[SM] %t", "Minimal Players Not Met");
				return Plugin_Continue;			
			}
			
			if (g_RTV_Voted[client])
			{
				PrintToChat(client, "[SM] %t", "Already Voted");
				return Plugin_Continue;
			}	
			
			new String:name[64];
			GetClientName(client, name, sizeof(name));
			
			g_RTV_Votes++;
			g_RTV_Voted[client] = true;
			
			PrintToChatAll("[SM] %t", "RTV Requested", name, g_RTV_Votes, g_RTV_VotesNeeded);
			
			if (g_RTV_Votes >= g_RTV_VotesNeeded)
			{
				g_RTV_Started = true;
				CreateTimer(2.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
			}
		}
		else if (GetConVarBool(g_Cvar_Nominate) && strcmp(text[startidx], "nominate", false) == 0)
		{
			if (g_MapChanged)
			{
				ReplyToCommand(client, "[SM] %t", "Map change in progress");
				return Plugin_Continue;
			}
			
			if (g_RTV_Started || g_HasVoteStarted)
			{
				ReplyToCommand(client, "[SM] %t", "Map vote in progress");
				return Plugin_Continue;				
			}
			
			if (g_Nominated[client])
			{
				PrintToChat(client, "[SM] %t", "Already Nominated");
				return Plugin_Continue;
			}
			
			if (GetArraySize(g_Nominate) >= GetConVarInt(g_Cvar_Maps))
			{
				PrintToChat(client, "[SM] %t", "Max Nominations");
				return Plugin_Continue;			
			}
			
			DisplayMenu(g_Menu_Nominate, client, MENU_TIME_FOREVER);		
		}		
	}
	
	return Plugin_Continue;	
}

public Action:Command_Mapvote(client, args)
{
	InitiateVote();

	return Plugin_Handled;	
}
 
public Action:Command_Nextmap(args)
{
	decl String:map[64];
	
	GetConVarString(g_Cvar_Nextmap, map, sizeof(map));
	
	ReplyToCommand(0, "%t", "Next Map", map);
	
	return Plugin_Handled;
} 
 
public Action:Command_List(client, args) 
{
	PrintToConsole(client, "Map Cycle:");
	
	decl String:currentMap[64];
	GetCurrentMap(currentMap, 64);	
	
	decl String:mapName[64];
	for (new i = 0; i < GetArraySize(g_MapCycle); i++)
	{
		GetArrayString(g_MapCycle, i, mapName, sizeof(mapName));
		if (strcmp(mapName, currentMap) == 0)
		{
			PrintToConsole(client, "%s <========= Current map", mapName);
		}
		else if (strcmp(mapName, g_NextMap) == 0)
		{
			PrintToConsole(client, "%s <========= Next map", mapName);
		}
		else
		{
			PrintToConsole(client, "%s", mapName);
		}
	}
 
	return Plugin_Handled;
} 

public Action:Command_SetNextmap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_setnextmap <map>");
		return Plugin_Handled;
	}
	
	if (g_MapChanged)
	{
		ReplyToCommand(client, "[SM] %t", "Map change in progress");
		return Plugin_Handled;
	}	

	decl String:map[64];
	GetCmdArg(1, map, sizeof(map));

	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}

	ShowActivity(client, "%t", "Cvar changed", "sm_nextmap", map);
	LogMessage("\"%L\" changed sm_nextmap to \"%s\"", client, map);

	SetNextMap(map);

	return Plugin_Handled;
}
 
 public Action:Command_Map(client, args)
 {
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_map <map> [r/e]");
		return Plugin_Handled;
	}
	
	if (g_MapChanged)
	{
		ReplyToCommand(client, "[SM] %t", "Map change in progress");
		return Plugin_Handled;
	}
 
	decl String:map[64];
	GetCmdArg(1, map, sizeof(map));
 
	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}
	
	decl String:when[2];
	if (args > 1)
	{
		GetCmdArg(2, when, sizeof(when));
		
		when[0] = CharToLower(when[0]);
		if (when[0] != 'r' && when[0] != 'e')
		{
			when[0] = 'i';
		}	
	}
	
	SetMapChange(client, map, when);
 
	return Plugin_Handled;
}

public Action:Command_Votemap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votemap [r/e] <mapname> [mapname2] ... [mapname5]");
		return Plugin_Handled;	
	}
	
	if (g_MapChanged)
	{
		ReplyToCommand(client, "[SM] %t", "Map change in progress");
		return Plugin_Handled;
	}	
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}
		
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	decl String:text[256];
	GetCmdArgString(text, sizeof(text));

	decl String:maps[5][64];
	new mapCount;	
	new len, pos;

	// Find out if the user specified "when"
	decl String:when[64];
	pos = BreakString(text[len], when, sizeof(when));
	if (!IsMapValid(when))
	{
		when[0] = CharToLower(when[0]);
		if (when[0] != 'r' && when[0] != 'e')
		{
			ReplyToCommand(client, "[SM] %t", "Map was not found", maps[mapCount]);
			return Plugin_Handled;
		}
	}
	else
	{
		strcpy(maps[mapCount], sizeof(maps[]), when);
		mapCount++;
		when[0] = 'i';
	}
	
	len += pos;
	
	while (pos != -1 && mapCount < 5)
	{	
		pos = BreakString(text[len], maps[mapCount], sizeof(maps[]));
		
		if (!IsMapValid(maps[mapCount]))
		{
			ReplyToCommand(client, "[SM] %t", "Map was not found", maps[mapCount]);
			return Plugin_Handled;
		}		

		mapCount++;
		
		len += pos;
	}

	g_VoteMapInUse = client;
	g_Client_Data[client][0] = when[0];

	DisplayVoteMapMenu(client, mapCount, maps);
	
	return Plugin_Handled;	
}

public Action:Command_Nominate(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_nominate <mapname>");
		return Plugin_Handled;
	}
	
	if (!GetConVarBool(g_Cvar_Nominate))
	{
		ReplyToCommand(client, "[SM] Nominations are currently disabled.");
		return Plugin_Handled;
	}
	
	decl String:mapname[64];
	GetCmdArg(1, mapname, sizeof(mapname));

	if (FindStringInArray(g_MapList, mapname) == -1)
	{
		ReplyToCommand(client, "%t", "Map was not found", mapname);
		return Plugin_Handled;
	}
	
	if (GetArraySize(g_Nominated) > 0)
	{
		if (FindStringInArray(g_Nominated, mapname) != -1)
		{
			ReplyToCommand(client, "%t", "Map Already In Vote", mapname);
			return Plugin_Handled;				
		}
		
		ShiftArrayUp(g_Nominated, 0);
		SetArrayString(g_Nominated, 0, mapname);
		
		while (GetArraySize(g_Nominated) > GetConVarInt(g_Cvar_Maps))
		{
			RemoveFromArray(g_Nominated, GetConVarInt(g_Cvar_Maps));
		}
	}
	else
	{
		PushArrayString(g_Nominated, mapname);
	}
		
	decl String:item[64];
	for (new i = 0; i < GetMenuItemCount(g_Menu_Nominate); i++)
	{
		GetMenuItem(g_Menu_Nominate, i, item, sizeof(item));
		if (strcmp(item, mapname) == 0)
		{
			RemoveMenuItem(g_Menu_Nominate, i);
			break;
		}			
	}	
	
	ReplyToCommand(client, "%t", "Map Inserted", mapname);
	LogAction(client, -1, "\"%L\" inserted map \"%s\".", client, mapname);

	return Plugin_Handled;		
}