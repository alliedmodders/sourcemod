/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides gravity functionality
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

int g_GravityTarget[MAXPLAYERS+1];

void PerformGravity(int client, int target, float amount)
{
	SetEntityGravity(target, amount);
	LogAction(client, target, "\"%L\" set gravity on \"%L\" (amount \"%f\")", client, target, amount);
}

public void AdminMenu_Gravity(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Gravity player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayGravityMenu(param);
	}
}

void DisplayGravityMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Gravity);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Gravity player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

void DisplayGravityAmountMenu(int client)
{
	Menu menu = new Menu(MenuHandler_GravityAmount);
	
	char title[100];
	Format(title, sizeof(title), "%T: %N", "Gravity amount", client, GetClientOfUserId(g_GravityTarget[client]));
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTranslatedMenuItem(menu, "4.0", "Gravity Very High", client);
	AddTranslatedMenuItem(menu, "2.0", "Gravity High", client);
	AddTranslatedMenuItem(menu, "1.0", "Gravity Normal", client);
	AddTranslatedMenuItem(menu, "0.5", "Gravity Low", client);
	AddTranslatedMenuItem(menu, "0.1", "Gravity Very Low", client);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Gravity(Menu menu, MenuAction action, int param1, int param2)
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
			g_GravityTarget[param1] = userid;
			DisplayGravityAmountMenu(param1);
			return 0;	// Return, because we went to a new menu and don't want the re-draw to occur.
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayGravityMenu(param1);
		}
	}

	return 0;
}

public int MenuHandler_GravityAmount(Menu menu, MenuAction action, int param1, int param2)
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
		float amount;
		int target;
		
		menu.GetItem(param2, info, sizeof(info));
		amount = StringToFloat(info);

		if ((target = GetClientOfUserId(g_GravityTarget[param1])) == 0)
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
			
			PerformGravity(param1, target, amount);
			ShowActivity2(param1, "[SM] ", "%t", "Set gravity on target", "_s", name, amount);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayGravityMenu(param1);
		}
	}

	return 0;
}

public Action Command_Gravity(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_gravity <#userid|name> [amount]");
		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	float amount = 1.0;
	if (args > 1)
	{
		if (!GetCmdArgFloatEx(2, amount))
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
		
		if (amount < 0.0)
		{
			amount = 0.0;
		}
	}

	char target_name[MAX_TARGET_LENGTH];
	int target_list[MAXPLAYERS], target_count;
	bool tn_is_ml;
	
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
	
	for (int i = 0; i < target_count; i++)
	{
		PerformGravity(client, target_list[i], amount);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Set gravity on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Set gravity on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
