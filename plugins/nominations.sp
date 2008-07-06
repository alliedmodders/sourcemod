/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Rock The Vote Plugin
 * Creates a map vote when the required number of players have requested one.
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
#include <mapchooser>

#pragma semicolon 1

public Plugin:myinfo =
{
	name = "Map Nominations",
	author = "AlliedModders LLC",
	description = "Provides Map Nominations",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_Cvar_ExcludeOld = INVALID_HANDLE;
new Handle:g_Cvar_ExcludeCurrent = INVALID_HANDLE;

new Handle:g_MapList = INVALID_HANDLE;
new Handle:g_MapMenu = INVALID_HANDLE;
new g_mapFileSerial = -1;

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("nominations.phrases");
	
	new arraySize = ByteCountToCells(33);	
	g_MapList = CreateArray(arraySize);
	
	g_Cvar_ExcludeOld = CreateConVar("sm_nominate_excludeold", "1", "Specifies if the current map should be excluded from the Nominations list", 0, true, 0.00, true, 1.0);
	g_Cvar_ExcludeCurrent = CreateConVar("sm_nominate_excludecurrent", "1", "Specifies if the MapChooser excluded maps should also be excluded from Nominations", 0, true, 0.00, true, 1.0);
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	RegConsoleCmd("sm_nominate", Command_Nominate);
	
	RegAdminCmd("sm_nominate_addmap", Command_Addmap, ADMFLAG_CHANGEMAP, "sm_nominate_addmap <mapname> - Forces a map to be on the next mapvote.");
}

public OnConfigsExecuted()
{
	if (ReadMapList(g_MapList,
					g_mapFileSerial,
					"nominations",
					MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER)
		== INVALID_HANDLE)
	{
		if (g_mapFileSerial == -1)
		{
			SetFailState("Unable to create a valid map list.");
		}
	}
	
	BuildMapMenu();
}

public OnNominationRemoved(String:map[], owner)
{
	AddMenuItem(g_MapMenu,	map, map);
}

public Action:Command_Addmap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_nominate_addmap <mapname>");
		return Plugin_Handled;
	}
	
	decl String:mapname[64];
	GetCmdArg(1, mapname, sizeof(mapname));

	if (FindStringInArray(g_MapList, mapname) == -1)
	{
		ReplyToCommand(client, "%t", "Map was not found", mapname);
		return Plugin_Handled;
	}
	
	new NominateResult:result = NominateMap(mapname, true, 0);
	
	if (result > Nominate_Replaced)
	{
		/* We assume already in vote is the casue because the maplist does a Map Validity check and we forced, so it can't be full */
		ReplyToCommand(client, "%t", "Map Already In Vote", mapname);
		
		return Plugin_Handled;	
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
	if (!client)
	{
		return Plugin_Continue;
	}

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
	
	new ReplySource:old = SetCmdReplySource(SM_REPLY_TO_CHAT);
	
	if (strcmp(text[startidx], "nominate", false) == 0)
	{
		AttemptNominate(client);
	}
	
	SetCmdReplySource(old);
	
	return Plugin_Continue;	
}

public Action:Command_Nominate(client, args)
{
	if (!client)
	{
		return Plugin_Continue;
	}
	
	if (args == 0)
	{
		AttemptNominate(client);
		return Plugin_Continue;
	}
	
	decl String:mapname[64];
	GetCmdArg(1, mapname, sizeof(mapname));
	
	decl String:item[64];
	for (new i = 0; i < GetMenuItemCount(g_MapMenu); i++)
	{
		GetMenuItem(g_MapMenu, i, item, sizeof(item));
		if (strcmp(item, mapname) == 0)
		{
			new NominateResult:result = NominateMap(mapname, false, client);
	
			if (result > Nominate_Replaced)
			{
				if (result == Nominate_AlreadyInVote)
				{
					ReplyToCommand(client, "%t", "Map Already In Vote", mapname);
				}
				else
				{
					ReplyToCommand(client, "[SM] %t", "Map Already Nominated");
				}
				
				return Plugin_Handled;	
			}
			
			RemoveMenuItem(g_MapMenu, i);
			
			decl String:name[64];
			GetClientName(client, name, sizeof(name));
	
			PrintToChatAll("[SM] %t", "Map Nominated", name, mapname);
	
			return Plugin_Continue;
		}			
	}
	
	ReplyToCommand(client, "%t", "Map was not found", mapname);
	return Plugin_Handled;
}

AttemptNominate(client)
{
	SetMenuTitle(g_MapMenu, "%t", "Nominate Title", client);
	DisplayMenu(g_MapMenu, client, MENU_TIME_FOREVER);
	
	return;
}

BuildMapMenu()
{
	if (g_MapMenu != INVALID_HANDLE)
	{
		CloseHandle(g_MapMenu);
		g_MapMenu = INVALID_HANDLE;
	}
	
	g_MapMenu = CreateMenu(Handler_MapSelectMenu);

	decl String:map[64];
	
	new Handle:excludeMaps;
	decl String:currentMap[32];
	
	if (GetConVarBool(g_Cvar_ExcludeOld))
	{	
		excludeMaps = CreateArray(ByteCountToCells(33));
		GetExcludeMapList(excludeMaps);
	}
	
	if (GetConVarBool(g_Cvar_ExcludeCurrent))
	{
		GetCurrentMap(currentMap, sizeof(currentMap));
	}
	
	for (new i = 0; i < GetArraySize(g_MapList); i++)
	{
		GetArrayString(g_MapList, i, map, sizeof(map));
		
		if (GetConVarBool(g_Cvar_ExcludeOld))
		{
			if (FindStringInArray(excludeMaps, map) != -1)
			{
				continue;	
			}
		}
		
		if (GetConVarBool(g_Cvar_ExcludeCurrent))
		{
			if (StrEqual(map, currentMap))
			{
				continue;
			}
		}
		
		AddMenuItem(g_MapMenu, map, map);
	}
	
	SetMenuExitButton(g_MapMenu, true);
}

public Handler_MapSelectMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			decl String:map[64], String:name[64];
			GetMenuItem(menu, param2, map, sizeof(map));
			
			
			GetClientName(param1, name, 64);
	
			
			new NominateResult:result = NominateMap(map, false, param1);
			
			/* Don't need to check for InvalidMap because the menu did that already */
			if (result == Nominate_AlreadyInVote)
			{
				PrintToChat(param1, "[SM] %t", "Map Already Nominated");
				return;
			}
			else if (result == Nominate_VoteFull)
			{
				PrintToChat(param1, "[SM] %t", "Max Nominations");
				return;
			}

			RemoveMenuItem(menu, param2);
			
			if (result == Nominate_Replaced)
			{
				PrintToChatAll("[SM] %t", "Map Nomination Changed", name, map);
				return;	
			}
			
			PrintToChatAll("[SM] %t", "Map Nominated", name, map);
		}
	}
}