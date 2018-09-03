/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands Plugin
 * Provides kick functionality
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
 
void PerformKick(int client, int target, const char[] reason)
{
	LogAction(client, target, "\"%L\" kicked \"%L\" (reason \"%s\")", client, target, reason);

	if (reason[0] == '\0')
	{
		KickClient(target, "%t", "Kicked by admin");
	}
	else
	{
		KickClient(target, "%s", reason);
	}
}

void DisplayKickMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Kick);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Kick player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, false, false);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public void AdminMenu_Kick(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Kick player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayKickMenu(param);
	}
}

public int MenuHandler_Kick(Menu menu, MenuAction action, int param1, int param2)
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
		int userid, target;
		
		menu.GetItem(param2, info, sizeof(info));
		userid = StringToInt(info);

		if ((target = GetClientOfUserId(userid)) == 0)
		{
			PrintToChat(param1, "[SM] %t", "Player no longer available");
		}
		else if (!CanUserTarget(param1, target))
		{
			PrintToChat(param1, "[SM] %t", "Unable to target");
		}
		else
		{
			char name[MAX_NAME_LENGTH];
			GetClientName(target, name, sizeof(name));
			ShowActivity2(param1, "[SM] ", "%t", "Kicked target", "_s", name);
			PerformKick(param1, target, "");
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayKickMenu(param1);
		}
	}
}

public Action Command_Kick(int client, int args)
{
	if (args < 1)
	{
		if ((GetCmdReplySource() == SM_REPLY_TO_CHAT) && (client != 0))
		{
			DisplayKickMenu(client);
		}
		else
		{
			ReplyToCommand(client, "[SM] Usage: sm_kick <#userid|name> [reason]");
		}
		
		return Plugin_Handled;
	}

	char Arguments[256];
	GetCmdArgString(Arguments, sizeof(Arguments));

	char arg[65];
	int len = BreakString(Arguments, arg, sizeof(arg));
	
	if (len == -1)
	{
		/* Safely null terminate */
		len = 0;
		Arguments[0] = '\0';
	}

	char target_name[MAX_TARGET_LENGTH];
	int target_list[MAXPLAYERS], target_count;
	bool tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client, 
			target_list, 
			MAXPLAYERS, 
			COMMAND_FILTER_CONNECTED,
			target_name,
			sizeof(target_name),
			tn_is_ml)) > 0)
	{
		char reason[64];
		Format(reason, sizeof(reason), Arguments[len]);

		if (tn_is_ml)
		{
			if (reason[0] == '\0')
			{
				ShowActivity2(client, "[SM] ", "%t", "Kicked target", target_name);
			}
			else
			{
				ShowActivity2(client, "[SM] ", "%t", "Kicked target reason", target_name, reason);
			}
		}
		else
		{
			if (reason[0] == '\0')
			{
				ShowActivity2(client, "[SM] ", "%t", "Kicked target", "_s", target_name);            
			}
			else
			{
				ShowActivity2(client, "[SM] ", "%t", "Kicked target reason", "_s", target_name, reason);
			}
		}
		
		int kick_self = 0;
		
		for (int i = 0; i < target_count; i++)
		{
			/* Kick everyone else first */
			if (target_list[i] == client)
			{
				kick_self = client;
			}
			else
			{
				PerformKick(client, target_list[i], reason);
			}
		}
		
		if (kick_self)
		{
			PerformKick(client, client, reason);
		}
	}
	else
	{
		ReplyToTargetError(client, target_count);
	}

	return Plugin_Handled;
}
