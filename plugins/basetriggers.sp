/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Info Triggers Plugin
 * Implements basic information chat triggers like ff and timeleft.
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

#pragma semicolon 1

#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Basic Info Triggers",
	author = "AlliedModders LLC",
	description = "Adds ff, timeleft, thetime, and others.",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_Cvar_TriggerShow = INVALID_HANDLE;
new Handle:g_Cvar_TimeleftInterval = INVALID_HANDLE;
new Handle:g_Cvar_FriendlyFire = INVALID_HANDLE;

new Handle:g_Timer_TimeShow = INVALID_HANDLE;

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basetriggers.phrases");
	
	g_Cvar_TriggerShow = CreateConVar("sm_trigger_show", "1", "Display triggers message to all players? (0 off, 1 on, def. 1)", 0, true, 0.0, true, 1.0);	
	g_Cvar_TimeleftInterval = CreateConVar("sm_timeleft_interval", "0.0", "Display timeleft every x seconds. Default 0.", 0, true, 0.0, true, 1800.0);
	g_Cvar_FriendlyFire = FindConVar("mp_friendlyfire");
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	HookConVarChange(g_Cvar_TimeleftInterval, ConVarChange_TimeleftInterval);
}

public ConVarChange_TimeleftInterval(Handle:convar, const String:oldValue[], const String:newValue[])
{
	new Float:newval = StringToFloat(newValue);
	
	if (newval < 1.0)
	{
		if (g_Timer_TimeShow != INVALID_HANDLE)
		{
			KillTimer(g_Timer_TimeShow);		
		}
		
		return;
	}
	
	if (g_Timer_TimeShow != INVALID_HANDLE)
	{
		KillTimer(g_Timer_TimeShow);
		g_Timer_TimeShow = CreateTimer(newval, Timer_DisplayTimeleft, _, TIMER_REPEAT);
	}
	else
		g_Timer_TimeShow = CreateTimer(newval, Timer_DisplayTimeleft, _, TIMER_REPEAT);
}

public Action:Timer_DisplayTimeleft(Handle:timer)
{
	new timeleft;
	if (GetMapTimeLeft(timeleft))
	{
		new mins, secs;
		
		if (timeleft > 0)
		{
			mins = timeleft / 60;
			secs = timeleft % 60;		
		}
		
		PrintToChatAll("[SM] %T %d:%02d", "Timeleft", LANG_SERVER, mins, secs);
	}
}

public Action:Command_Say(client, args)
{
	decl String:text[192], String:command[64];
	new startidx = 0;
	if (GetCmdArgString(text, sizeof(text)) < 1)
	{
		return Plugin_Continue;
	}
	
	if (text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}

	if (strcmp(command, "say2", false) == 0)
		startidx += 4;

	if (strcmp(text[startidx], "timeleft", false) == 0)
	{
		new timeleft;
		if (GetMapTimeLeft(timeleft))
		{
			new mins, secs;
			
			if (timeleft > 0)
			{
				mins = timeleft / 60;
				secs = timeleft % 60;		
			}
			
			if(GetConVarInt(g_Cvar_TriggerShow))
			{
				PrintToChatAll("[SM] %t %d:%02d", "Timeleft", mins, secs);
			}
			else
			{
				PrintToChat(client,"[SM] %t %d:%02d", "Timeleft", mins, secs);
			}
		}
		
	}
	else if (strcmp(text[startidx], "thetime", false) == 0)
	{
		decl String:ctime[64];
		FormatTime(ctime, 64, NULL_STRING);
		
		if(GetConVarInt(g_Cvar_TriggerShow))
		{
			PrintToChatAll("[SM] %t", "Thetime", ctime);
		}
		else
		{
			PrintToChat(client,"[SM] %t", "Thetime", ctime);
		}
	}
	else if (strcmp(text[startidx], "ff", false) == 0 || strcmp(text[startidx], "/ff", false) == 0)
	{
		if (g_Cvar_FriendlyFire != INVALID_HANDLE)
		{
			decl String:message[64];
			if (GetConVarBool(g_Cvar_FriendlyFire))
			{
				Format(message, sizeof(message), "%T", "Friendly Fire On", client);
			}
			else
			{
				Format(message, sizeof(message), "%T", "Friendly Fire Off", client);
			}
		
			if(GetConVarInt(g_Cvar_TriggerShow))
			{
				PrintToChatAll("[SM] %s", message);
			}
			else
			{
				PrintToChat(client,"[SM] %s", message);		
			}
		}
	}
	else if (strcmp(text[startidx], "currentmap", false) == 0)
	{
		decl String:map[64];
		GetCurrentMap(map, sizeof(map));
		
		if(GetConVarInt(g_Cvar_TriggerShow))
		{
			PrintToChatAll("[SM] %t", "Current Map", map);
		}
		else
		{
			PrintToChat(client,"[SM] %t", "Current Map", map);
		}
	}	
	
	return Plugin_Continue;
}