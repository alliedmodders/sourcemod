/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides slap functionality
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
 
new g_SlapDamage[MAXPLAYERS+1];

PerformSlap(client, target, damage)
{
	LogAction(client, target, "\"%L\" slapped \"%L\" (damage \"%d\")", client, target, damage);
	SlapPlayer(target, damage, true);
}

DisplaySlapDamageMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_SlapDamage);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Slap damage", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	menu.AddItem("0", "0");
	menu.AddItem("1", "1");
	menu.AddItem("5", "5");
	menu.AddItem("10", "10");
	menu.AddItem("20", "20");
	menu.AddItem("50", "50");
	menu.AddItem("99", "99");
	
	menu.Display(client, MENU_TIME_FOREVER);
}

DisplaySlapTargetMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_Slap);
	
	char title[100];
	Format(title, sizeof(title), "%T: %d damage", "Slap player", client, g_SlapDamage[client]);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public AdminMenu_Slap(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Slap player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplaySlapDamageMenu(param);
	}
}

public MenuHandler_SlapDamage(Menu menu, MenuAction action, param1, param2)
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
		g_SlapDamage[param1] = StringToInt(info);
		
		DisplaySlapTargetMenu(param1);
	}
}

public MenuHandler_Slap(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		char info[32];
		new userid, target;
		
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
		else if (!IsPlayerAlive(target))
		{
			ReplyToCommand(param1, "[SM] %t", "Player has since died");
		}	
		else
		{
			decl String:name[32];
			GetClientName(target, name, sizeof(name));
			PerformSlap(param1, target, g_SlapDamage[param1]);
			ShowActivity2(param1, "[SM] ", "%t", "Slapped target", "_s", name);
		}
		
		DisplaySlapTargetMenu(param1);
	}
}

public Action:Command_Slap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_slap <#userid|name> [damage]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new damage = 0;
	if (args > 1)
	{
		decl String:arg2[20];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToIntEx(arg2, damage) == 0 || damage < 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
	}
	
	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[MAXPLAYERS], target_count, bool:tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_ALIVE,
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
	
	for (new i = 0; i < target_count; i++)
	{
		PerformSlap(client, target_list[i], damage);
	}

	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Slapped target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Slapped target", "_s", target_name);
	}

	return Plugin_Handled;
}
