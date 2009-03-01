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

new g_GravityTarget[MAXPLAYERS+1];

PerformGravity(client, target, Float:amount)
{
	new offset = FindDataMapOffs(client, "m_flGravity");
	new Float:temp = GetEntDataFloat(client, offset);
	PrintToChatAll("Gravity check %f", temp);
	
	SetEntityGravity(target, amount);
	LogAction(client, target, "\"%L\" set gravity on \"%L\" to %d.", client, target, amount);
}

public AdminMenu_Gravity(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
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

DisplayGravityMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Gravity);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Gravity player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayGravityAmountMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_GravityAmount);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T: %N", "Gravity amount", client, GetClientOfUserId(g_GravityTarget[client]));
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "4.0", "Gravity Very High");
	AddMenuItem(menu, "2.0", "Gravity High");
	AddMenuItem(menu, "1.0", "Gravity Normal");
	AddMenuItem(menu, "0.5", "Gravity Low");
	AddMenuItem(menu, "0.1", "Gravity Very Low");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_Gravity(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
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
			g_GravityTarget[param1] = userid;
			DisplayGravityAmountMenu(param1);
			return;	// Return, because we went to a new menu and don't want the re-draw to occur.
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayGravityMenu(param1);
		}
	}
	
	return;
}

public MenuHandler_GravityAmount(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32];
		new Float:amount, target;
		
		GetMenuItem(menu, param2, info, sizeof(info));
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
			new String:name[32];
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
}

public Action:Command_Gravity(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_gravity <#userid|name> [amount]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new Float:amount = 1.0;
	if (args > 1)
	{
		decl String:arg2[20];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToFloatEx(arg2, amount) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
		
		if (amount < 0.0)
		{
			amount = 0.0;
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
