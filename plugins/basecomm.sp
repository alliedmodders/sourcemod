/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Communication Plugin
 * Provides fucntionality for controlling communication on the server
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
#include <sdktools>

#pragma semicolon 1

public Plugin:myinfo =
{
	name = "Basic Comm Control",
	author = "AlliedModders LLC",
	description = "Provides methods of controllingg communication.",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new bool:g_Muted[MAXPLAYERS+1];		// Is the player muted?
new bool:g_Gagged[MAXPLAYERS+1];	// Is the player gagged?

new Handle:g_Cvar_Deadtalk = INVALID_HANDLE;	// Holds the handle for sm_deadtalk
new Handle:g_Cvar_Alltalk = INVALID_HANDLE;	// Holds the handle for sv_alltalk
new bool:g_Hooked = false;			// Tracks if we've hooked events for deadtalk

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basecomm.phrases");
	
	g_Cvar_Deadtalk = CreateConVar("sm_deadtalk", "0", "Controls how dead communicate. 0 - Off. 1 - Dead players ignore teams. 2 - Dead players talk to living teammates.", 0, true, 0.0, true, 2.0);
	g_Cvar_Alltalk = FindConVar("sv_alltalk");
	
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);
	
	RegAdminCmd("sm_mute", Command_Mute, ADMFLAG_CHAT, "sm_mute <player> - Removes a player's ability to use voice.");
	RegAdminCmd("sm_gag", Command_Gag, ADMFLAG_CHAT, "sm_gag <player> - Removes a player's ability to use chat.");
	RegAdminCmd("sm_silence", Command_Silence, ADMFLAG_CHAT, "sm_silence <player> - Removes a player's ability to use voice or chat.");
	
	RegAdminCmd("sm_unmute", Command_Unmute, ADMFLAG_CHAT, "sm_unmute <player> - Restores a player's ability to use voice.");
	RegAdminCmd("sm_ungag", Command_Ungag, ADMFLAG_CHAT, "sm_ungag <player> - Restores a player's ability to use chat.");
	RegAdminCmd("sm_unsilence", Command_Unsilence, ADMFLAG_CHAT, "sm_unsilence <player> - Restores a player's ability to use voice and chat.");	
	
	HookConVarChange(g_Cvar_Deadtalk, ConVarChange_Deadtalk);
}

public ConVarChange_Deadtalk(Handle:convar, const String:oldValue[], const String:newValue[])
{
	if (GetConVarInt(g_Cvar_Deadtalk))
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


public bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	g_Gagged[client] = false;
	g_Muted[client] = false;
	
	return true;
}

public Action:Command_Say(client, args)
{
	if (client)
	{
		if (g_Gagged[client])
		{
			return Plugin_Handled;		
		}
	}
	
	return Plugin_Continue;
}

public Action:Command_Mute(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_mute <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Already Muted");
		return Plugin_Handled;		
	}
		
	g_Muted[target] = true;
	SetClientListeningFlags(target, VOICE_MUTED);
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Muted", name);
	ReplyToCommand(client, "%t", "Player Muted", name);
	LogAction(client, target, "\"%L\" muted \"%L\"", client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Gag(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_gag <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (g_Gagged[target])
	{
		ReplyToCommand(client, "%t", "Already Gagged");
		return Plugin_Handled;		
	}
		
	g_Gagged[target] = true;
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Gagged", name);
	ReplyToCommand(client, "%t", "Player Gagged", name);
	LogAction(client, target, "\"%L\" gagged \"%L\"", client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Silence(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_silence <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (g_Gagged[target] && g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Already Silenced");
		return Plugin_Handled;		
	}

	decl String:name[64];
	GetClientName(target, name, sizeof(name));
	
	if (!g_Gagged[target])
	{
		g_Gagged[target] = true;

		ShowActivity(client, "%t", "Player Gagged", name);
		ReplyToCommand(client, "%t", "Player Gagged", name);
		LogAction(client, target, "\"%L\" gagged \"%L\"", client, target);
	}
	
	if (!g_Muted[target])
	{
		g_Muted[target] = true;
		SetClientListeningFlags(target, VOICE_MUTED);
	
		ShowActivity(client, "%t", "Player Muted", name);
		ReplyToCommand(client, "%t", "Player Muted", name);
		LogAction(client, target, "\"%L\" muted \"%L\"", client, target);
	}
	
	return Plugin_Handled;	
}

public Action:Command_Unmute(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unmute <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (!g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Player Not Muted");
		return Plugin_Handled;		
	}
		
	g_Muted[target] = false;
	if (GetConVarInt(g_Cvar_Deadtalk) == 1 && !IsPlayerAlive(target))
	{
		SetClientListeningFlags(target, VOICE_LISTENALL);
	}
	else if (GetConVarInt(g_Cvar_Deadtalk) == 2 && !IsPlayerAlive(target))
	{
		SetClientListeningFlags(target, VOICE_TEAM);
	}
	else
	{
		SetClientListeningFlags(target, VOICE_NORMAL);
	}
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Unmuted", name);
	ReplyToCommand(client, "%t", "Player Unmuted", name);
	LogAction(client, target, "\"%L\" unmuted \"%L\"", client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Ungag(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_ungag <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (!g_Gagged[target])
	{
		ReplyToCommand(client, "%t", "Player Not Gagged");
		return Plugin_Handled;		
	}
		
	g_Gagged[target] = false;
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Ungagged", name);
	ReplyToCommand(client, "%t", "Player Ungagged", name);
	LogAction(client, target, "\"%L\" ungagged \"%L\"", client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Unsilence(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unsilence <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (!g_Gagged[target] && !g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Player Not Silenced");
		return Plugin_Handled;		
	}
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));
	
	if (g_Gagged[target])
	{
		g_Gagged[target] = false;
		
		ShowActivity(client, "%t", "Player Ungagged", name);
		ReplyToCommand(client, "%t", "Player Ungagged", name);	
		LogAction(client, target, "\"%L\" ungagged \"%L\"", client, target);
	}
	
	if (g_Muted[target])
	{
		g_Muted[target] = false;
		
		if (GetConVarInt(g_Cvar_Deadtalk) == 1 && !IsPlayerAlive(target))
		{
			SetClientListeningFlags(target, VOICE_LISTENALL);
		}
		else if (GetConVarInt(g_Cvar_Deadtalk) == 2 && !IsPlayerAlive(target))
		{
			SetClientListeningFlags(target, VOICE_TEAM);
		}
		else
		{
			SetClientListeningFlags(target, VOICE_NORMAL);
		}
		
		ShowActivity(client, "%t", "Player Unmuted", name);
		ReplyToCommand(client, "%t", "Player Unmuted", name);
		LogAction(client, target, "\"%L\" unmuted \"%L\"", client, target);		
	}
	
	return Plugin_Handled;	
}

public Event_PlayerSpawn(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	
	if (g_Muted[client])
	{
		SetClientListeningFlags(client, VOICE_MUTED);
	}
	else
	{
		SetClientListeningFlags(client, VOICE_NORMAL);
	}
}

public Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	
	if (g_Muted[client])
	{
		SetClientListeningFlags(client, VOICE_MUTED);
		return;
	}
	
	if (GetConVarBool(g_Cvar_Alltalk))
	{
		SetClientListeningFlags(client, VOICE_NORMAL);
		return;
	}
	
	if (GetConVarInt(g_Cvar_Deadtalk) == 1)
	{
		SetClientListeningFlags(client, VOICE_LISTENALL);
	}
	else if (GetConVarInt(g_Cvar_Deadtalk) == 2)
	{
		SetClientListeningFlags(client, VOICE_TEAM);
	}
}