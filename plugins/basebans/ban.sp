/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands
 * Functionality related to banning.
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

// Because GetTargetName will return either client name, or client authid,
// and we need to be able to fit whichever is larger to a buffer.
#if MAX_AUTHID_LENGTH > MAX_NAME_LENGTH
#define MAX_TARGETNAME_LENGTH MAX_AUTHID_LENGTH
#else
#define MAX_TARGETNAME_LENGTH MAX_NAME_LENGTH
#endif

void PrepareBan(int adminClient, int time, const char[] reason)
{
	char name[MAX_TARGETNAME_LENGTH];
	GetTargetName(playerinfo[adminClient].banTargetUserId, playerinfo[adminClient].banTargetAuthId, name, sizeof(name));

	if (!time)
	{
		if (reason[0] == '\0')
		{
			ShowActivity(adminClient, "%t", "Permabanned player", name);
		}
		else
		{
			ShowActivity(adminClient, "%t", "Permabanned player reason", name, reason);
		}
	}
	else
	{
		if (reason[0] == '\0')
		{
			ShowActivity(adminClient, "%t", "Banned player", name, time);
		}
		else
		{
			ShowActivity(adminClient, "%t", "Banned player reason", name, time, reason);
		}
	}

	int target = (playerinfo[adminClient].banTargetUserId == 0)
					? 0 : GetClientOfUserId(playerinfo[adminClient].banTargetUserId);

	if (target == 0)
	{
		target = GetClientOfAuthId(playerinfo[adminClient].banTargetAuthId);
	}

	// Ban & kick if target is connected, else record the ban with authid
	if (target != 0)
	{
		LogAction(adminClient,
				target,
				"\"%L\" banned \"%L\" (minutes \"%d\") (reason \"%s\")",
				adminClient,
				target,
				time,
				reason);

		BanClient(target, time, BANFLAG_AUTO,
				(reason[0] == '\0') ? "Banned" : reason,
				(reason[0] == '\0') ? "Banned" : reason,
				"sm_ban", adminClient);
	}
	else
	{
		LogAction(adminClient,
			  target,
			  "\"%L\" added ban (minutes \"%d\") (id \"%s\") (reason \"%s\")",
			  adminClient,
			  time,
			  playerinfo[adminClient].banTargetAuthId,
			  reason);

		BanIdentity(playerinfo[adminClient].banTargetAuthId, time, BANFLAG_AUTHID,
				(reason[0] == '\0') ? "Banned" : reason,
				"sm_addban", adminClient);
	}
}

void DisplayBanTargetMenu(int client)
{
	Menu menu = new Menu(MenuHandler_BanPlayerList);

	char title[100];
	Format(title, sizeof(title), "%T:", "Ban player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = CheckCommandAccess(client, "sm_admin", ADMFLAG_GENERIC, false);

	AddTargetsToMenuByAuthId(menu, client, COMMAND_FILTER_NO_BOTS|COMMAND_FILTER_CONNECTED, AuthId_Steam2);

	menu.Display(client, MENU_TIME_FOREVER);
}

void DisplayBanTimeMenu(int client)
{
	char targetName[MAX_TARGETNAME_LENGTH];
	GetTargetName(playerinfo[client].banTargetUserId, playerinfo[client].banTargetAuthId, targetName, sizeof(targetName));

	Menu menu = new Menu(MenuHandler_BanTimeList);

	char title[100];
	Format(title, sizeof(title), "%T: %s", "Ban player", client, targetName);
	menu.SetTitle(title);
	menu.ExitBackButton = true;

	menu.AddItem("0", "Permanent");
	menu.AddItem("10", "10 Minutes");
	menu.AddItem("30", "30 Minutes");
	menu.AddItem("60", "1 Hour");
	menu.AddItem("240", "4 Hours");
	menu.AddItem("1440", "1 Day");
	menu.AddItem("10080", "1 Week");

	menu.Display(client, MENU_TIME_FOREVER);
}

void DisplayBanReasonMenu(int client)
{
	char name[MAX_TARGETNAME_LENGTH];
	GetTargetName(playerinfo[client].banTargetUserId, playerinfo[client].banTargetAuthId, name, sizeof(name));

	Menu menu = new Menu(MenuHandler_BanReasonList);

	char title[100];
	Format(title, sizeof(title), "%T: %s", "Ban reason", client, name);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	//Add custom chat reason entry first
	menu.AddItem("", "Custom reason (type in chat)");
	
	//Loading configurable entries from the kv-file
	char reasonName[100];
	char reasonFull[255];
	
	//Iterate through the kv-file
	g_hKvBanReasons.GotoFirstSubKey(false);
	do
	{
		g_hKvBanReasons.GetSectionName(reasonName, sizeof(reasonName));
		g_hKvBanReasons.GetString(NULL_STRING, reasonFull, sizeof(reasonFull));
		
		//Add entry
		menu.AddItem(reasonFull, reasonName);
		
	} while (g_hKvBanReasons.GotoNextKey(false));
	
	//Reset kvHandle
	g_hKvBanReasons.Rewind();

	menu.Display(client, MENU_TIME_FOREVER);
}

public void AdminMenu_Ban(TopMenu topmenu,
							  TopMenuAction action,
							  TopMenuObject object_id,
							  int param,
							  char[] buffer,
							  int maxlength)
{
	//Reset chat reason first
	playerinfo[param].isWaitingForChatReason = false;
	
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Ban player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBanTargetMenu(param);
	}
}

public int MenuHandler_BanReasonList(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		if(param2 == 0)
		{
			//Chat reason
			playerinfo[param1].isWaitingForChatReason = true;
			PrintToChat(param1, "[SM] %t", "Custom ban reason explanation", "sm_abortban");
		}
		else
		{
			char info[64];

			menu.GetItem(param2, info, sizeof(info));

			PrepareBan(param1, playerinfo[param1].banTime, info);
		}
	}

	return 0;
}

public int MenuHandler_BanPlayerList(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		char name[32];
		menu.GetItem(param2, playerinfo[param1].banTargetAuthId, sizeof(PlayerInfo::banTargetAuthId), _, name, sizeof(name));

		int target = GetClientOfAuthId(playerinfo[param1].banTargetAuthId);
		playerinfo[param1].banTargetUserId = (target == 0) ? 0 : GetClientUserId(target);

		DisplayBanTimeMenu(param1);
	}

	return 0;
}

public int MenuHandler_BanTimeList(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		char info[32];

		menu.GetItem(param2, info, sizeof(info));
		playerinfo[param1].banTime = StringToInt(info);

		DisplayBanReasonMenu(param1);
	}

	return 0;
}

public Action Command_Ban(int client, int args)
{
	if (args < 2)
	{
		if ((GetCmdReplySource() == SM_REPLY_TO_CHAT) && (client != 0) && (args == 0))
		{
			DisplayBanTargetMenu(client);
		}
		else
		{
			ReplyToCommand(client, "[SM] Usage: sm_ban <#userid|name> <minutes|0> [reason]");
		}
		
		return Plugin_Handled;
	}

	int len, next_len;
	char Arguments[256];
	GetCmdArgString(Arguments, sizeof(Arguments));

	char arg[65];
	len = BreakString(Arguments, arg, sizeof(arg));

	int target = FindTarget(client, arg, true);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	char s_time[12];
	if ((next_len = BreakString(Arguments[len], s_time, sizeof(s_time))) != -1)
	{
		len += next_len;
	}
	else
	{
		len = 0;
		Arguments[0] = '\0';
	}

	playerinfo[client].banTargetUserId = GetClientUserId(target);
	GetClientAuthId(target, AuthId_Steam2, playerinfo[client].banTargetAuthId, sizeof(PlayerInfo::banTargetAuthId));

	int time = StringToInt(s_time);
	PrepareBan(client, time, Arguments[len]);

	return Plugin_Handled;
}

int GetClientOfAuthId(const char[] authid, AuthIdType authIdType=AuthId_Steam2)
{
	char compareId[MAX_AUTHID_LENGTH];
	for (int client = 1; client <= MaxClients; client++)
	{
		if (!IsClientInGame(client) || !IsClientAuthorized(client))
		{
			continue;
		}
		if (!GetClientAuthId(client, authIdType, compareId, sizeof(compareId)))
		{
			continue;
		}
		if (StrEqual(compareId, authid))
		{
			return client;
		}
	}

	return 0;
}

void GetTargetName(int userid, const char[] authid, char[] out, int maxlen)
{
	int client = GetClientOfUserId(userid);
	if (client)
	{
		Format(out, maxlen, "%N", client);
	}
	else
	{
		// if client was not in game, use their authid as the name instead
		strcopy(out, maxlen, authid);
	}
}

/**
 * Adds targets to an admin menu.
 *
 * Each client is displayed as: name (authid)
 * Each item contains the authid as a string for its info.
 *
 * @param menu          Menu Handle.
 * @param source_client Source client, or 0 to ignore immunity.
 * @param flags         COMMAND_FILTER flags from commandfilters.inc.
 * @return              Number of clients added.
 */
int AddTargetsToMenuByAuthId(Menu menu, int source_client, int flags, AuthIdType authIdType)
{
	char authid[MAX_AUTHID_LENGTH];
	char name[MAX_NAME_LENGTH];
	char display[sizeof(name) + sizeof(authid) + 3];

	int num_clients;

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientConnected(i) || IsClientInKickQueue(i))
		{
			continue;
		}

		if (((flags & COMMAND_FILTER_NO_BOTS) == COMMAND_FILTER_NO_BOTS)
			&& IsFakeClient(i))
		{
			continue;
		}

		if (((flags & COMMAND_FILTER_CONNECTED) != COMMAND_FILTER_CONNECTED)
			&& !IsClientInGame(i))
		{
			continue;
		}

		if (((flags & COMMAND_FILTER_ALIVE) == COMMAND_FILTER_ALIVE)
			&& !IsPlayerAlive(i))
		{
			continue;
		}

		if (((flags & COMMAND_FILTER_DEAD) == COMMAND_FILTER_DEAD)
			&& IsPlayerAlive(i))
		{
			continue;
		}

		if ((source_client && ((flags & COMMAND_FILTER_NO_IMMUNITY) != COMMAND_FILTER_NO_IMMUNITY))
			&& !CanUserTarget(source_client, i))
		{
			continue;
		}

		GetClientAuthId(i, authIdType, authid, sizeof(authid));
		GetClientName(i, name, sizeof(name));
		Format(display, sizeof(display), "%s (%s)", name, authid);
		menu.AddItem(authid, display);
		num_clients++;
	}

	return num_clients;
}