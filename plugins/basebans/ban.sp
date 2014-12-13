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

PrepareBan(client, target, time, const String:reason[])
{
	new originalTarget = GetClientOfUserId(g_BanTargetUserId[client]);

	if (originalTarget != target)
	{
		if (client == 0)
		{
			PrintToServer("[SM] %t", "Player no longer available");
		}
		else
		{
			PrintToChat(client, "[SM] %t", "Player no longer available");
		}

		return;
	}

	new String:name[32];
	GetClientName(target, name, sizeof(name));

	if (!time)
	{
		if (reason[0] == '\0')
		{
			ShowActivity(client, "%t", "Permabanned player", name);
		} else {
			ShowActivity(client, "%t", "Permabanned player reason", name, reason);
		}
	} else {
		if (reason[0] == '\0')
		{
			ShowActivity(client, "%t", "Banned player", name, time);
		} else {
			ShowActivity(client, "%t", "Banned player reason", name, time, reason);
		}
	}

	LogAction(client, target, "\"%L\" banned \"%L\" (minutes \"%d\") (reason \"%s\")", client, target, time, reason);

	if (reason[0] == '\0')
	{
		BanClient(target, time, BANFLAG_AUTO, "Banned", "Banned", "sm_ban", client);
	}
	else
	{
		BanClient(target, time, BANFLAG_AUTO, reason, reason, "sm_ban", client);
	}
}

DisplayBanTargetMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_BanPlayerList);

	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Ban player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;

	AddTargetsToMenu2(menu, client, COMMAND_FILTER_NO_BOTS|COMMAND_FILTER_CONNECTED);

	menu.Display(client, MENU_TIME_FOREVER);
}

DisplayBanTimeMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_BanTimeList);

	decl String:title[100];
	Format(title, sizeof(title), "%T: %N", "Ban player", client, g_BanTarget[client]);
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

DisplayBanReasonMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_BanReasonList);

	decl String:title[100];
	Format(title, sizeof(title), "%T: %N", "Ban reason", client, g_BanTarget[client]);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	//Add custom chat reason entry first
	menu.AddItem("", "Custom reason (type in chat)");
	
	//Loading configurable entries from the kv-file
	decl String:reasonName[100];
	decl String:reasonFull[255];
	
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

public AdminMenu_Ban(Handle:topmenu,
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	//Reset chat reason first
	g_IsWaitingForChatReason[param] = false;
	
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Ban player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBanTargetMenu(param);
	}
}

public MenuHandler_BanReasonList(Menu menu, MenuAction action, int param1, int param2)
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
			g_IsWaitingForChatReason[param1] = true;
			PrintToChat(param1, "[SM] %t", "Custom ban reason explanation", "sm_abortban");
		}
		else
		{
			decl String:info[64];
			
			menu.GetItem(param2, info, sizeof(info));
			
			PrepareBan(param1, g_BanTarget[param1], g_BanTime[param1], info);
		}
	}
}

public MenuHandler_BanPlayerList(Menu menu, MenuAction action, int param1, int param2)
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
		decl String:info[32], String:name[32];
		new userid, target;

		menu.GetItem(param2, info, sizeof(info), _, name, sizeof(name));
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
			g_BanTarget[param1] = target;
			g_BanTargetUserId[param1] = userid;
			DisplayBanTimeMenu(param1);
		}
	}
}

public MenuHandler_BanTimeList(Menu menu, MenuAction action, int param1, int param2)
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
		decl String:info[32];

		menu.GetItem(param2, info, sizeof(info));
		g_BanTime[param1] = StringToInt(info);

		DisplayBanReasonMenu(param1);
	}
}


public Action:Command_Ban(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_ban <#userid|name> <minutes|0> [reason]");
		return Plugin_Handled;
	}

	decl len, next_len;
	decl String:Arguments[256];
	GetCmdArgString(Arguments, sizeof(Arguments));

	decl String:arg[65];
	len = BreakString(Arguments, arg, sizeof(arg));

	new target = FindTarget(client, arg, true);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	decl String:s_time[12];
	if ((next_len = BreakString(Arguments[len], s_time, sizeof(s_time))) != -1)
	{
		len += next_len;
	}
	else
	{
		len = 0;
		Arguments[0] = '\0';
	}

	new time = StringToInt(s_time);

	g_BanTargetUserId[client] = GetClientUserId(target);

	PrepareBan(client, target, time, Arguments[len]);

	return Plugin_Handled;
}
