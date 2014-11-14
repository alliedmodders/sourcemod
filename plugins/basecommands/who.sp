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
 
PerformWho(client, target, ReplySource:reply, bool:is_admin)
{
	decl String:name[MAX_NAME_LENGTH];
	GetClientName(target, name, sizeof(name));
	
	new bool:show_name = false;
	new String:admin_name[MAX_NAME_LENGTH];
	new AdminId:id = GetUserAdmin(target);
	if (id != INVALID_ADMIN_ID && GetAdminUsername(id, admin_name, sizeof(admin_name)))
	{
		show_name = true;
	}
	
	new ReplySource:old_reply = SetCmdReplySource(reply);
	
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
			new flags = GetUserFlagBits(target);
			decl String:flagstring[255];
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

DisplayWhoMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Who);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Identify player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu2(menu, 0, COMMAND_FILTER_CONNECTED);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Who(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
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

public MenuHandler_Who(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
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
		decl String:info[32];
		new userid, target;
		
		GetMenuItem(menu, param2, info, sizeof(info));
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
public Action:Command_Who(client, args)
{
	new bool:is_admin = false;
	
	if (!client || (client && GetUserFlagBits(client) != 0))
	{
		is_admin = true;
	}
	
	if (args < 1)
	{
		/* Display header */
		decl String:t_access[16], String:t_name[16], String:t_username[16];
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
		decl String:flagstring[255];

		for (new i=1; i<=MaxClients; i++)
		{
			if (!IsClientInGame(i))
			{
				continue;
			}
			new flags = GetUserFlagBits(i);
			new AdminId:id = GetUserAdmin(i);
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
			decl String:name[MAX_NAME_LENGTH];
			new String:username[MAX_NAME_LENGTH];
			
			GetClientName(i, name, sizeof(name));
			
			if (id != INVALID_ADMIN_ID)
			{
				GetAdminUsername(id, username, sizeof(username));
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

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new target = FindTarget(client, arg, false, false);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	PerformWho(client, target, GetCmdReplySource(), is_admin);

	return Plugin_Handled;
}
