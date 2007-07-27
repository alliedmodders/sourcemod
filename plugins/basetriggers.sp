/**
 * basetriggers.sp
 * Implements basic information chat triggers like ff and timeleft
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
	name = "Basic Info Triggers",
	author = "AlliedModders LLC",
	description = "Adds ff, timeleft, thetime, and others.",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Float:g_fStartTime;
new Handle:g_Cvar_TriggerShow = INVALID_HANDLE;
new Handle:g_Cvar_TimeleftInterval = INVALID_HANDLE;
new Handle:g_Cvar_Timelimit = INVALID_HANDLE;
new Handle:g_Cvar_FriendlyFire = INVALID_HANDLE;

new Handle:g_Timer_TimeShow = INVALID_HANDLE;

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basetriggers.phrases");
	
	g_Cvar_TriggerShow = CreateConVar("sm_trigger_show", "1", "Display timeleft to all players? (0 off, 1 on, def. 1)", 0, true, 0.0, true, 1.0);	
	g_Cvar_TimeleftInterval = CreateConVar("sm_timeleft_interval", "0.0", "Display timeleft every x seconds. Default 0.", 0, true, 0.0, true, 1800.0);
	g_Cvar_Timelimit = FindConVar("mp_timelimit");
	g_Cvar_FriendlyFire = FindConVar("mp_friendlyfire");
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	HookEvent("round_end", Event_RoundEnd, EventHookMode_Post);
	
	HookConVarChange(g_Cvar_TimeleftInterval, ConVarChange_TimeleftInterval);
}

public ConVarChange_TimeleftInterval(Handle:convar, const String:oldValue[], const String:newValue[])
{
	new Float:newval = StringToFloat(newValue);
	
	if (newval < 1.0 && g_Timer_TimeShow != INVALID_HANDLE)
	{
		KillTimer(g_Timer_TimeShow);		
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
	new mins, secs;
	new timeleft = RoundToNearest(GetTimeLeft());
	
	if (timeleft > 0)
	{
		mins = timeleft / 60;
		secs = timeleft % 60;		
	}

	PrintToChatAll("[SM] %T %d:%02d", "Timeleft", LANG_SERVER, mins, secs);
}

public Action:Command_Say(client, args)
{
	decl String:text[192], String:command[64];
	new startidx = 0;
	GetCmdArgString(text, sizeof(text));
	
	if (text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}

	if (strcmp(command, "say2", false) == 0)
		startidx += 4;

	if (strcmp(text[startidx], "timeleft", false) == 0)
	{
		new mins, secs;
		new timeleft = RoundToNearest(GetTimeLeft());
		
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
	else if (strcmp(text[startidx], "thetime", false) == 0)
	{
		decl String:ctime[64];
		FormatTime(ctime, 64, "%m/%d/%Y - %H:%M:%S");
		
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

public Event_RoundEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
	new reason = GetEventInt(event, "reason");
	if(reason == 16)
	{
		g_fStartTime = GetEngineTime();		
	}	
}

public OnMapStart()
{
	g_fStartTime = GetEngineTime();
}

Float:GetTimeLeft()
{
	new Float:fLimit = GetConVarFloat(g_Cvar_Timelimit);
	new Float:fElapsed = GetEngineTime() - g_fStartTime;
	
	return (fLimit*60.0) - fElapsed;
}
