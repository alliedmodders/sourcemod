/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands Plugin
 * Provides sm_who functionality
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
 
void PerformWho(int client, int target, ReplySource reply, bool is_admin)
{
	char name[MAX_NAME_LENGTH];
	GetClientName(target, name, sizeof(name));
	
	bool show_name = false;
	char admin_name[MAX_NAME_LENGTH];
	AdminId id = GetUserAdmin(target);
	if (id != INVALID_ADMIN_ID && id.GetUsername(admin_name, sizeof(admin_name)))
	{
		show_name = true;
	}
	
	ReplySource old_reply = SetCmdReplySource(reply);
	
	if (id == INVALID_ADMIN_ID)
	{
		ReplyToCommand(client, "[SM] %t", "Player is not an admin", name);
	}
	else
	{
		if (!is_admin)
		{
			ReplyToCommand(client, "[SM] %t", "Player is an admin", name);
		}
		else
		{
			int flags = GetUserFlagBits(target);
			char flagstring[255];
			if (flags == 0)
			{
				strcopy(flagstring, sizeof(flagstring), "none");
			}
			else if (flags & ADMFLAG_ROOT)
			{
				strcopy(flagstring, sizeof(flagstring), "root");
			}
			else
			{
				FlagsToString(flagstring, sizeof(flagstring), flags);
			}
			
			if (show_name)
			{
				ReplyToCommand(client, "[SM] %t", "Admin logged in as", name, admin_name, flagstring);
			}
			else
			{
				ReplyToCommand(client, "[SM] %t", "Admin logged in anon", name, flagstring);
			}
		}
	}
	
	SetCmdReplySource(old_reply);
}

void DisplayWhoMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Who);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Identify player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu2(menu, 0, COMMAND_FILTER_CONNECTED);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public void AdminMenu_Who(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Identify player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayWhoMenu(param);
	}
}

public int MenuHandler_Who(Menu menu, MenuAction action, int param1, int param2)
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
			PerformWho(param1, target, SM_REPLY_TO_CHAT, (GetUserFlagBits(param1) != 0 ? true : false));
		}
		
		/* Re-draw the menu if they're still valid */
		
		/* - Close the menu? redisplay? jump back up to the category?
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayWhoMenu(param1);
		}
		*/
	}
}
public Action Command_Who(int client, int args)
{
	bool is_admin = false;
	
	if (!client || (client && GetUserFlagBits(client) != 0))
	{
		is_admin = true;
	}
	
	if (args < 1)
	{
		/* Display header */
		char t_access[16], t_name[16], t_username[16];
		Format(t_access, sizeof(t_access), "%T", "Admin access", client);
		Format(t_name, sizeof(t_name), "%T", "Name", client);
		Format(t_username, sizeof(t_username), "%T", "Username", client);

		if (is_admin)
		{
			PrintToConsole(client, "    %-24.23s %-18.17s %s", t_name, t_username, t_access);
		}
		else
		{
			PrintToConsole(client, "    %-24.23s %s", t_name, t_access);
		}

		/* List all players */
		char flagstring[255];

		for (int i=1; i<=MaxClients; i++)
		{
			if (!IsClientInGame(i))
			{
				continue;
			}
			int flags = GetUserFlagBits(i);
			AdminId id = GetUserAdmin(i);
			if (flags == 0)
			{
				strcopy(flagstring, sizeof(flagstring), "none");
			}
			else if (flags & ADMFLAG_ROOT)
			{
				strcopy(flagstring, sizeof(flagstring), "root");
			}
			else
			{
				FlagsToString(flagstring, sizeof(flagstring), flags);
			}
			char name[MAX_NAME_LENGTH];
			char username[MAX_NAME_LENGTH];
			
			GetClientName(i, name, sizeof(name));
			
			if (id != INVALID_ADMIN_ID)
			{
				id.GetUsername(username, sizeof(username));
			}
			
			if (is_admin)
			{
				PrintToConsole(client, "%2d. %-24.23s %-18.17s %s", i, name, username, flagstring);
			}
			else
			{
				if (flags == 0)
				{
					PrintToConsole(client, "%2d. %-24.23s %t", i, name, "No");
				}
				else
				{
					PrintToConsole(client, "%2d. %-24.23s %t", i, name, "Yes");
				}
			}
		}

		if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
		{
			ReplyToCommand(client, "[SM] %t", "See console for output");
		}

		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	int target = FindTarget(client, arg, false, false);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	PerformWho(client, target, GetCmdReplySource(), is_admin);

	return Plugin_Handled;
}
