/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Nextmap Plugin
 * Adds sm_nextmap cvar for changing map and nextmap chat trigger.
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
	name = "Nextmap",
	author = "AlliedModders LLC",
	description = "Provides nextmap and sm_nextmap",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new bool:g_IntermissionCalled;
new UserMsg:g_VGUIMenu;
 
new Handle:g_Cvar_Chattime;
new Handle:g_Cvar_Nextmap;
new Handle:g_Cvar_Mapcycle;

new g_MapPos = -1;
new Handle:g_MapList = INVALID_HANDLE;
new g_mapFileTime;
 
public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("nextmap.phrases");
	
	g_VGUIMenu = GetUserMessageId("VGUIMenu");
	if (g_VGUIMenu == INVALID_MESSAGE_ID)
	{
		LogError("FATAL: Cannot find VGUIMenu user message id. Nextmap not loaded.");
		SetFailState("VGUIMenu Not Found");
	}
	
	g_Cvar_Chattime = FindConVar("mp_chattime");
	g_Cvar_Mapcycle = FindConVar("mapcyclefile");
	
	g_MapList = CreateArray(32);

	if (!LoadMaps())
	{
		LogError("FATAL: Cannot load map cycle. Nextmap not loaded.");
		SetFailState("Mapcycle Not Found");		
	}	
	
	HookUserMessage(g_VGUIMenu, UserMsg_VGUIMenu);
	HookConVarChange(g_Cvar_Mapcycle, ConVarChange_Mapcyclefile);
	
	g_Cvar_Nextmap = CreateConVar("sm_nextmap", "", "Sets the Next Map", FCVAR_NOTIFY);
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);

	RegAdminCmd("sm_setnextmap", Command_SetNextmap, ADMFLAG_CHANGEMAP, "sm_setnextmap <map>");
	RegConsoleCmd("listmaps", Command_List);

	// Set to the current map so OnMapStart() will know what to do
	decl String:currentMap[64];
	GetCurrentMap(currentMap, 64);
	SetConVarString(g_Cvar_Nextmap, currentMap);
}
 
public OnMapStart()
{
	decl String:lastMap[64], String:currentMap[64];
	GetConVarString(g_Cvar_Nextmap, lastMap, 64);
	GetCurrentMap(currentMap, 64);
	
	// Why am I doing this? If we switched to a new map, but it wasn't what we expected (Due to sm_map, sm_votemap, or
	// some other plugin/command), we don't want to scramble the map cycle. Or for example, admin switches to a custom map
	// not in mapcyclefile. So we keep it set to the last expected nextmap. - ferret
	if (strcmp(lastMap, currentMap) == 0)
	{
		if (!LoadMaps())
		{
			LogError("FATAL: Cannot load map cycle. Nextmap not loaded.");
			SetFailState("Mapcycle Not Found");	
		}		
		
		FindAndSetNextMap();
	}
}
 
public OnMapEnd()
{
	g_IntermissionCalled = false;
}
 
public ConVarChange_Mapcyclefile(Handle:convar, const String:oldValue[], const String:newValue[])
{
	if (strcmp(oldValue, newValue, false) != 0)
	{
		if (!LoadMaps())
		{
			LogError("FATAL: Cannot load map cycle. Nextmap not loaded.");
			SetFailState("Mapcycle Not Found");	
		}
		
		FindAndSetNextMap();
	}
}
 
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
		
		PrintToChatAll("%t", "Next Map", map);
	}
	
	return Plugin_Continue;	
}

public Action:Command_SetNextmap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_setnextmap <map>");
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

	SetConVarString(g_Cvar_Nextmap, map);

	return Plugin_Handled;
}

public Action:Command_List(client, args) 
{
	PrintToConsole(client, "Map Cycle:");
	
	new mapCount = GetArraySize(g_MapList);
	decl String:mapName[32];
	for (new i = 0; i < mapCount; i++)
	{
		GetArrayString(g_MapList, i, mapName, sizeof(mapName));
		PrintToConsole(client, "%s", mapName);
	}
 
	return Plugin_Handled;
}
 
public Action:UserMsg_VGUIMenu(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	if (g_IntermissionCalled)
	{
		return Plugin_Handled;
	}
	
	decl String:type[15];
	BfReadString(bf, type, sizeof(type));
 
	if (BfReadByte(bf) == 1 && BfReadByte(bf) == 0 && (strcmp(type, "scores", false) == 0))
	{
		g_IntermissionCalled = true;
		
		decl String:map[32];
		new Float:fChatTime = GetConVarFloat(g_Cvar_Chattime);
		
		GetConVarString(g_Cvar_Nextmap, map, sizeof(map));
		
		if (fChatTime < 2.0)
			SetConVarFloat(g_Cvar_Chattime, 2.0);
		
		new Handle:dp;
		CreateDataTimer(fChatTime - 1.0, Timer_ChangeMap, dp);
		WritePackString(dp, map);
	}
	
	return Plugin_Handled;
}
 
public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	new String:map[32];
	
	ResetPack(dp);
	ReadPackString(dp, map, sizeof(map));
 
	InsertServerCommand("changelevel \"%s\"", map);
	ServerExecute();
	
	LogMessage("Nextmap changed map to \"%s\"", map);
	
	return Plugin_Stop;
}
 
LoadMaps()
{
	decl String:mapCycle[64];
	GetConVarString(g_Cvar_Mapcycle, mapCycle, 64);	
	
	if (!FileExists(mapCycle))
	{
		return 0;
	}
	
	new fileTime =  GetFileTime(mapCycle, FileTime_LastChange);
	if (g_mapFileTime == fileTime)
	{
		return GetArraySize(g_MapList);
	}
	
	g_mapFileTime = fileTime;
 
	new Handle:file = OpenFile(mapCycle, "r");
	
	if (file == INVALID_HANDLE)
	{
		LogError("[SM] Could not open file: %s", mapCycle);
		return 0;
	}
	
	g_MapPos = -1;
	if (g_MapList != INVALID_HANDLE)
	{
		ClearArray(g_MapList);
	}
	
	decl String:buffer[255];
	while (!IsEndOfFile(file) && ReadFileLine(file, buffer, sizeof(buffer)))
	{
		TrimString(buffer);
		if (buffer[0] == '\0' || buffer[0] == ';')
		{
			continue;
		}
		
		if (IsMapValid(buffer))
		{
			PushArrayString(g_MapList, buffer);
		}
	}
	
	CloseHandle(file);

	return GetArraySize(g_MapList);
}
 
FindAndSetNextMap()
{
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
