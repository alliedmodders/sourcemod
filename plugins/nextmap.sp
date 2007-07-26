/**
 * nextmap.sp
 * Adds sm_nextmap Cvar for changing map, and nextmap chat trigger
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */
 
#include <sourcemod>
 
#pragma semicolon 1
 
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

#if 0
new bool:g_INS = false;
#endif
 
public OnPluginStart()
{
	LoadTranslations("plugin.nextmap");
	
	g_VGUIMenu = GetUserMessageId("VGUIMenu");
	if (g_VGUIMenu == INVALID_MESSAGE_ID)
	{
		LogMessage("FATAL: Cannot find VGUIMenu user message id. Nextmap not loaded.");
		return;	
	}
	
	g_Cvar_Chattime = FindConVar("mp_chattime");
	g_Cvar_Mapcycle = FindConVar("mapcyclefile");
	
	g_MapList = CreateArray(32);
	
	decl String:mapCycle[64];
	GetConVarString(g_Cvar_Mapcycle, mapCycle, 64);
	if (LoadMaps(mapCycle) == 0)
	{
		LogMessage("FATAL: Cannot load map cycle. Nextmap not loaded.");
		return;
	}	
	
	HookUserMessage(g_VGUIMenu, UserMsg_VGUIMenu);
	HookConVarChange(g_Cvar_Mapcycle, ConVarChange_Mapcyclefile);
	
	g_Cvar_Nextmap = CreateConVar("sm_nextmap", "", "Sets the Next Map", FCVAR_NOTIFY);
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	#if 0
	decl String:modname[64];
	GetGameFolderName(modname, sizeof(modname));
	
	if (strcmp(modname, "ins") == 0)
	{
		RegConsoleCmd("say2", Command_SayChat);
		g_INS = true;
	}
	#endif

	RegConsoleCmd("listmaps", Command_List);
	
	/* Set to the current map so OnMapStart() will know what to do */
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
		LoadMaps(newValue);
	}
}
 
public Action:Command_Say(client, args)
{
	decl String:text[192];
	GetCmdArgString(text, sizeof(text));
	
	new startidx;
	if (text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}
	
	#if 0
	if (g_INS)
	{
		startidx += 4;
	}
	#endif
	
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
 
LoadMaps(const String:path[])
{
	if (!FileExists(path))
	{
		return 0;
	}
 
	new Handle:file = OpenFile(path, "r");
	
	if (file == INVALID_HANDLE)
	{
		LogError("[SM] Could not open file: %s", path);
		return 0;
	}
	
	ClearArray(g_MapList);
	g_MapPos = -1;
	
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