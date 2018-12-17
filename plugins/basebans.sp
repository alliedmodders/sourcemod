/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Commands Plugin
 * Implements basic admin commands.
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
#undef REQUIRE_PLUGIN
#include <adminmenu>

#pragma newdecls required

public Plugin myinfo =
{
	name = "Basic Ban Commands",
	author = "AlliedModders LLC",
	description = "Basic Banning Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

TopMenu hTopMenu;

enum struct PlayerInfo {
	int banTarget;
	int banTargetUserId;
	int banTime;
	int isWaitingForChatReason;
}

PlayerInfo playerinfo[MAXPLAYERS+1];

KeyValues g_hKvBanReasons;
char g_BanReasonsPath[PLATFORM_MAX_PATH];

#include "basebans/ban.sp"

public void OnPluginStart()
{
	BuildPath(Path_SM, g_BanReasonsPath, sizeof(g_BanReasonsPath), "configs/banreasons.txt");

	LoadBanReasons();

	LoadTranslations("common.phrases");
	LoadTranslations("basebans.phrases");
	LoadTranslations("core.phrases");

	RegAdminCmd("sm_ban", Command_Ban, ADMFLAG_BAN, "sm_ban <#userid|name> <minutes|0> [reason]");
	RegAdminCmd("sm_unban", Command_Unban, ADMFLAG_UNBAN, "sm_unban <steamid|ip>");
	RegAdminCmd("sm_addban", Command_AddBan, ADMFLAG_RCON, "sm_addban <time> <steamid> [reason]");
	RegAdminCmd("sm_banip", Command_BanIp, ADMFLAG_BAN, "sm_banip <ip|#userid|name> <time> [reason]");
	
	//This to manage custom ban reason messages
	RegConsoleCmd("sm_abortban", Command_AbortBan, "sm_abortban");
	
	/* Account for late loading */
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
	{
		OnAdminMenuReady(topmenu);
	}
}

public void OnConfigsExecuted()
{
	//(Re-)Load BanReasons
	LoadBanReasons();
}

public void OnClientDisconnect(int client)
{
	playerinfo[client].isWaitingForChatReason = false;
}

void LoadBanReasons()
{
	delete g_hKvBanReasons;

	g_hKvBanReasons = new KeyValues("banreasons");

	if (g_hKvBanReasons.ImportFromFile(g_BanReasonsPath))
	{
		char sectionName[255];
		if (!g_hKvBanReasons.GetSectionName(sectionName, sizeof(sectionName)))
		{
			SetFailState("Error in %s: File corrupt or in the wrong format", g_BanReasonsPath);
			return;
		}

		if (strcmp(sectionName, "banreasons") != 0)
		{
			SetFailState("Error in %s: Couldn't find 'banreasons'", g_BanReasonsPath);
			return;
		}
		
		//Reset kvHandle
		g_hKvBanReasons.Rewind();
	} else {
		SetFailState("Error in %s: File not found, corrupt or in the wrong format", g_BanReasonsPath);
		return;
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
	
	/* Find the "Player Commands" category */
	TopMenuObject player_commands = hTopMenu.FindCategory(ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_ban", AdminMenu_Ban, player_commands, "sm_ban", ADMFLAG_BAN);
	}
}

public Action Command_BanIp(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_banip <ip|#userid|name> <time> [reason]");
		return Plugin_Handled;
	}

	int len, next_len;
	char Arguments[256];
	char arg[50], time[20];
	
	GetCmdArgString(Arguments, sizeof(Arguments));
	len = BreakString(Arguments, arg, sizeof(arg));
	
	if ((next_len = BreakString(Arguments[len], time, sizeof(time))) != -1)
	{
		len += next_len;
	}
	else
	{
		len = 0;
		Arguments[0] = '\0';
	}

	if (StrEqual(arg, "0"))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot ban that IP");
		return Plugin_Handled;
	}
	
	char target_name[MAX_TARGET_LENGTH];
	int target_list[1];
	bool tn_is_ml;
	int found_client = -1;
	
	if (ProcessTargetString(
			arg,
			client, 
			target_list, 
			1, 
			COMMAND_FILTER_CONNECTED|COMMAND_FILTER_NO_MULTI,
			target_name,
			sizeof(target_name),
			tn_is_ml) > 0)
	{
		found_client = target_list[0];
	}
	
	bool has_rcon;
	
	if (client == 0 || (client == 1 && !IsDedicatedServer()))
	{
		has_rcon = true;
	}
	else
	{
		AdminId id = GetUserAdmin(client);
		has_rcon = (id == INVALID_ADMIN_ID) ? false : GetAdminFlag(id, Admin_RCON);
	}
	
	int hit_client = -1;
	if (found_client != -1
		&& !IsFakeClient(found_client)
		&& (has_rcon || CanUserTarget(client, found_client)))
	{
		GetClientIP(found_client, arg, sizeof(arg));
		hit_client = found_client;
	}
	
	if (hit_client == -1 && !has_rcon)
	{
		ReplyToCommand(client, "[SM] %t", "No Access");
		return Plugin_Handled;
	}

	int minutes = StringToInt(time);

	LogAction(client, 
			  hit_client, 
			  "\"%L\" added ban (minutes \"%d\") (ip \"%s\") (reason \"%s\")", 
			  client, 
			  minutes, 
			  arg, 
			  Arguments[len]);
				
	ReplyToCommand(client, "[SM] %t", "Ban added");
	
	BanIdentity(arg, 
				minutes, 
				BANFLAG_IP, 
				Arguments[len], 
				"sm_banip", 
				client);
				
	if (hit_client != -1)
	{
		KickClient(hit_client, "Banned: %s", Arguments[len]);
	}

	return Plugin_Handled;
}

public Action Command_AddBan(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_addban <time> <steamid> [reason]");
		return Plugin_Handled;
	}

	char arg_string[256];
	char time[50];
	char authid[50];

	GetCmdArgString(arg_string, sizeof(arg_string));

	int len, total_len;

	/* Get time */
	if ((len = BreakString(arg_string, time, sizeof(time))) == -1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_addban <time> <steamid> [reason]");
		return Plugin_Handled;
	}	
	total_len += len;

	/* Get steamid */
	if ((len = BreakString(arg_string[total_len], authid, sizeof(authid))) != -1)
	{
		total_len += len;
	}
	else
	{
		total_len = 0;
		arg_string[0] = '\0';
	}

	/* Verify steamid */
	bool idValid = false;
	if (!strncmp(authid, "STEAM_", 6) && authid[7] == ':')
		idValid = true;
	else if (!strncmp(authid, "[U:", 3))
		idValid = true;
	
	if (!idValid)
	{
		ReplyToCommand(client, "[SM] %t", "Invalid SteamID specified");
		return Plugin_Handled;
	}

	AdminId tid = FindAdminByIdentity("steam", authid);
	if (client && !CanAdminTarget(GetUserAdmin(client), tid))
	{
		ReplyToCommand(client, "[SM] %t", "No Access");
		return Plugin_Handled;
	}

	int minutes = StringToInt(time);

	LogAction(client, 
			  -1, 
			  "\"%L\" added ban (minutes \"%d\") (id \"%s\") (reason \"%s\")", 
			  client, 
			  minutes, 
			  authid, 
			  arg_string[total_len]);
	BanIdentity(authid, 
				minutes, 
				BANFLAG_AUTHID, 
				arg_string[total_len], 
				"sm_addban", 
				client);

	ReplyToCommand(client, "[SM] %t", "Ban added");

	return Plugin_Handled;
}

public Action Command_Unban(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unban <steamid|ip>");
		return Plugin_Handled;
	}

	char arg[50];
	GetCmdArgString(arg, sizeof(arg));

	ReplaceString(arg, sizeof(arg), "\"", "");	

	int ban_flags;
	if (IsCharNumeric(arg[0]))
	{
		ban_flags |= BANFLAG_IP;
	}
	else
	{
		ban_flags |= BANFLAG_AUTHID;
	}

	LogAction(client, -1, "\"%L\" removed ban (filter \"%s\")", client, arg);
	RemoveBan(arg, ban_flags, "sm_unban", client);

	ReplyToCommand(client, "[SM] %t", "Removed bans matching", arg);

	return Plugin_Handled;
}

public Action Command_AbortBan(int client, int args)
{
	if(!CheckCommandAccess(client, "sm_ban", ADMFLAG_BAN))
	{
		ReplyToCommand(client, "[SM] %t", "No Access");
		return Plugin_Handled;
	}
	if(playerinfo[client].isWaitingForChatReason)
	{
		playerinfo[client].isWaitingForChatReason = false;
		ReplyToCommand(client, "[SM] %t", "AbortBan applied successfully");
	}
	else
	{
		ReplyToCommand(client, "[SM] %t", "AbortBan not waiting for custom reason");
	}
	
	return Plugin_Handled;
}

public Action OnClientSayCommand(int client, const char[] command, const char[] sArgs)
{
	if(playerinfo[client].isWaitingForChatReason)
	{
		playerinfo[client].isWaitingForChatReason = false;
		PrepareBan(client, playerinfo[client].banTarget, playerinfo[client].banTime, sArgs);
		return Plugin_Stop;
	}

	return Plugin_Continue;
}
