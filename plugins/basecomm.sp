/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Communication Plugin
 * Provides fucntionality for controlling communication on the server
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 1
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
#include <sdktools>
#undef REQUIRE_PLUGIN
#include <adminmenu>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo =
{
	name = "Basic Comm Control",
	author = "AlliedModders LLC",
	description = "Provides methods of controlling communication.",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

enum struct PlayerState {
	bool isMuted; // Is the player muted?
	bool isGagged; // Is the player gagged?
	int gagTarget;
}

PlayerState playerstate[MAXPLAYERS+1];

ConVar g_Cvar_Deadtalk;				// Holds the handle for sm_deadtalk
ConVar g_Cvar_Alltalk;				// Holds the handle for sv_alltalk
bool g_Hooked = false;				// Tracks if we've hooked events for deadtalk

TopMenu hTopMenu;

#include "basecomm/gag.sp"
#include "basecomm/natives.sp"
#include "basecomm/forwards.sp"

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("BaseComm_IsClientGagged", Native_IsClientGagged);
	CreateNative("BaseComm_IsClientMuted",  Native_IsClientMuted);
	CreateNative("BaseComm_SetClientGag",   Native_SetClientGag);
	CreateNative("BaseComm_SetClientMute",  Native_SetClientMute);
	RegPluginLibrary("basecomm");
	
	return APLRes_Success;
}

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basecomm.phrases");
	
	g_Cvar_Deadtalk = CreateConVar("sm_deadtalk", "0", "Controls how dead communicate. 0 - Off. 1 - Dead players ignore teams. 2 - Dead players talk to living teammates.", 0, true, 0.0, true, 2.0);
	g_Cvar_Alltalk = FindConVar("sv_alltalk");
	
	RegAdminCmd("sm_mute", Command_Mute, ADMFLAG_CHAT, "sm_mute <player> - Removes a player's ability to use voice.");
	RegAdminCmd("sm_gag", Command_Gag, ADMFLAG_CHAT, "sm_gag <player> - Removes a player's ability to use chat.");
	RegAdminCmd("sm_silence", Command_Silence, ADMFLAG_CHAT, "sm_silence <player> - Removes a player's ability to use voice or chat.");
	
	RegAdminCmd("sm_unmute", Command_Unmute, ADMFLAG_CHAT, "sm_unmute <player> - Restores a player's ability to use voice.");
	RegAdminCmd("sm_ungag", Command_Ungag, ADMFLAG_CHAT, "sm_ungag <player> - Restores a player's ability to use chat.");
	RegAdminCmd("sm_unsilence", Command_Unsilence, ADMFLAG_CHAT, "sm_unsilence <player> - Restores a player's ability to use voice and chat.");	
	
	g_Cvar_Deadtalk.AddChangeHook(ConVarChange_Deadtalk);

	if (g_Cvar_Alltalk) {
		g_Cvar_Alltalk.AddChangeHook(ConVarChange_Alltalk);
	}
	
	/* Account for late loading */
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
	{
		OnAdminMenuReady(topmenu);
	}
}

public void OnAdminMenuReady(Handle aTopMenu)
{
	TopMenu topmenu = TopMenu.FromHandle(aTopMenu);

	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Build the "Player Commands" category */
	TopMenuObject player_commands = hTopMenu.FindCategory(ADMINMENU_PLAYERCOMMANDS);
	
	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_gag", AdminMenu_Gag, player_commands, "sm_gag", ADMFLAG_CHAT);
	}
}

public void ConVarChange_Deadtalk(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (g_Cvar_Deadtalk.IntValue)
	{
		HookEvent("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
		HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
		g_Hooked = true;
	}
	else if (g_Hooked)
	{
		UnhookEvent("player_spawn", Event_PlayerSpawn);
		UnhookEvent("player_death", Event_PlayerDeath);		
		g_Hooked = false;
	}
}

public bool OnClientConnect(int client, char[] rejectmsg, int maxlen)
{
	playerstate[client].isGagged = false;
	playerstate[client].isMuted = false;
	
	return true;
}

public Action OnClientSayCommand(int client, const char[] command, const char[] sArgs)
{
	if (client && playerstate[client].isGagged)
	{
		return Plugin_Stop;
	}
	
	return Plugin_Continue;
}

public void ConVarChange_Alltalk(ConVar convar, const char[] oldValue, const char[] newValue)
{
	int mode = g_Cvar_Deadtalk.IntValue;
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
		{
			continue;
		}
		
		if (playerstate[i].isMuted)
		{
			SetClientListeningFlags(i, VOICE_MUTED);
		}
		else if (g_Cvar_Alltalk.BoolValue)
		{
			SetClientListeningFlags(i, VOICE_NORMAL);
		}
		else if (!IsPlayerAlive(i))
		{
			if (mode == 1)
			{
				SetClientListeningFlags(i, VOICE_LISTENALL);
			}
			else if (mode == 2)
			{
				SetClientListeningFlags(i, VOICE_TEAM);
			}
		}
	}
}

public void Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	
	if (!client)
	{
		return;	
	}
	
	if (playerstate[client].isMuted)
	{
		SetClientListeningFlags(client, VOICE_MUTED);
	}
	else
	{
		SetClientListeningFlags(client, VOICE_NORMAL);
	}
}

public void Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	
	if (!client)
	{
		return;	
	}
	
	if (playerstate[client].isMuted)
	{
		SetClientListeningFlags(client, VOICE_MUTED);
		return;
	}
	
	if (g_Cvar_Alltalk && g_Cvar_Alltalk.BoolValue)
	{
		SetClientListeningFlags(client, VOICE_NORMAL);
		return;
	}
	
	int mode = g_Cvar_Deadtalk.IntValue;
	if (mode == 1)
	{
		SetClientListeningFlags(client, VOICE_LISTENALL);
	}
	else if (mode == 2)
	{
		SetClientListeningFlags(client, VOICE_TEAM);
	}
}
