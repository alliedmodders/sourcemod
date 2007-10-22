/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Commands Plugin
 * Implements basic admin commands.
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
#undef REQUIRE_PLUGIN
#include <adminmenu>

public Plugin:myinfo =
{
	name = "Basic Ban Commands",
	author = "AlliedModders LLC",
	description = "Basic Banning Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:hTopMenu = INVALID_HANDLE;

new g_BanTarget[MAXPLAYERS+1];
new g_BanTime[MAXPLAYERS+1];

#include "basebans/ban.sp"

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basebans.phrases");

	RegAdminCmd("sm_ban", Command_Ban, ADMFLAG_BAN, "sm_ban <#userid|name> <minutes|0> [reason]");
	RegAdminCmd("sm_unban", Command_Unban, ADMFLAG_UNBAN, "sm_unban <steamid>");
	RegAdminCmd("sm_addban", Command_AddBan, ADMFLAG_RCON, "sm_addban <time> <steamid> [reason]");
	RegAdminCmd("sm_banip", Command_BanIp, ADMFLAG_BAN, "sm_banip <time> <ip|#userid|name> [reason]");
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Find the "Player Commands" category */
	new TopMenuObject:player_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_ban",
			TopMenuObject_Item,
			AdminMenu_Ban,
			player_commands,
			"sm_ban",
			ADMFLAG_BAN);
			
	}
}

public Action:Command_BanIp(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_banip <time> <ip|#userid|name> [reason]");
		return Plugin_Handled;
	}

	decl String:arg[50], String:time[20];
	GetCmdArg(1, time, sizeof(time));
	GetCmdArg(2, arg, sizeof(arg));
	
	if (StrEqual(arg, "0"))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot ban that IP");
		return Plugin_Handled;
	}
	
	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[1], bool:tn_is_ml;
	new found_client = -1;
	
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
	
	new bool:has_rcon;
	
	if (client == 0 || (client == 1 && !IsDedicatedServer()))
	{
		has_rcon = true;
	}
	else
	{
		new AdminId:id = GetUserAdmin(client);
		has_rcon = (id == INVALID_ADMIN_ID) ? false : GetAdminFlag(id, Admin_RCON);
	}
	
	new hit_client = -1;
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

	new minutes = StringToInt(time);

	new String:reason[128];
	if (args >= 3)
	{
		GetCmdArg(3, reason, sizeof(reason));
	}

	LogAction(client, 
			  hit_client, 
			  "\"%L\" added ban (minutes \"%d\") (ip \"%s\") (reason \"%s\")", 
			  client, 
			  minutes, 
			  arg, 
			  reason);
				
	ReplyToCommand(client, "[SM] %t", "Ban added");
	
	BanIdentity(arg, 
				minutes, 
				BANFLAG_IP, 
				reason, 
				"sm_banip", 
				client);
				
	if (hit_client != -1)
	{
		KickClient(hit_client, "Banned: %s", reason);
	}

	return Plugin_Handled;
}

public Action:Command_AddBan(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_addban <time> <steamid> [reason]");
		return Plugin_Handled;
	}

	decl String:arg_string[256];
	new String:reason[128];
	new String:time[50];
	new String:authid[50];

	GetCmdArgString(arg_string, sizeof(arg_string));

	new len, total_len;

	/* Get time */
	if ((len = BreakString(arg_string, time, sizeof(time))) == -1)
	{
		ReplyToCommand(client, "t[SM] Usage: sm_addban <time> <steamid> [reason]");
		return Plugin_Handled;
	}	
	total_len += len;

	/* Get steamid */
	if ((len = BreakString(arg_string[total_len], authid, sizeof(authid))) != -1)
	{
		/* Get reason */
		total_len += len;
		BreakString(arg_string[total_len], reason, sizeof(reason));
	}

	/* Verify steamid */
	if (strncmp(authid, "STEAM_0:", 8) != 0)
	{
		ReplyToCommand(client, "[SM] %t", "Invalid SteamID specified");
		return Plugin_Handled;
	}

	new minutes = StringToInt(time);

	LogAction(client, 
			  -1, 
			  "\"%L\" added ban (minutes \"%d\") (id \"%s\") (reason \"%s\")", 
			  client, 
			  minutes, 
			  authid, 
			  reason);
	BanIdentity(authid, 
				minutes, 
				BANFLAG_AUTHID, 
				reason, 
				"sm_addban", 
				client);

	ReplyToCommand(client, "[SM] %t", "Ban added");

	return Plugin_Handled;
}

public Action:Command_Unban(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unban <steamid|ip>");
		return Plugin_Handled;
	}

	decl String:arg[50];
	GetCmdArgString(arg, sizeof(arg));

	ReplaceString(arg, sizeof(arg), "\"", "");	

	new ban_flags;
	if (strncmp(arg, "STEAM_0:", 8) == 0)
	{
		ban_flags |= BANFLAG_AUTHID;
	}
	else
	{
		ban_flags |= BANFLAG_IP;
	}

	LogAction(client, -1, "\"%L\" removed ban (filter \"%s\")", client, arg);
	RemoveBan(arg, ban_flags, "sm_unban", client);

	ReplyToCommand(client, "[SM] %t", "Removed bans matching", arg);

	return Plugin_Handled;
}

