/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Rename Plugin
 * Provides renaming functionality
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

char g_NewName[MAXPLAYERS+1][MAX_NAME_LENGTH];

void PerformRename(int client, int target)
{
	LogAction(client, target, "\"%L\" renamed \"%L\" (to \"%s\")", client, target, g_NewName[target]);

	SetClientName(target, g_NewName[target]);

	g_NewName[target][0] = '\0';
}

public void AdminMenu_Rename(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Rename player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayRenameTargetMenu(param);
	}
}

void DisplayRenameTargetMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Rename);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Rename player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Rename(Menu menu, MenuAction action, int param1, int param2)
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

			RandomizeName(target);
			ShowActivity2(param1, "[SM] ", "%t", "Renamed target", "_s", name);
			PerformRename(param1, target);
		}		
		DisplayRenameTargetMenu(param1);
	}
}

void RandomizeName(int client)
{
	char name[MAX_NAME_LENGTH];
	GetClientName(client, name, sizeof(name));

	int len = strlen(name);
	g_NewName[client][0] = '\0';

	for (int i = 0; i < len; i++)
	{
		g_NewName[client][i] = name[GetRandomInt(0, len - 1)];
	}
	g_NewName[client][len] = '\0';
}

public Action Command_Rename(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_rename <#userid|name> [newname]");
		return Plugin_Handled;
	}

	char arg[MAX_NAME_LENGTH], arg2[MAX_NAME_LENGTH];
	GetCmdArg(1, arg, sizeof(arg));

	bool randomize;
	if (args > 1)
	{
		GetCmdArg(2, arg2, sizeof(arg2));
	}
	else
	{
		randomize = true;
	}
	
	char target_name[MAX_TARGET_LENGTH];
	int target_list[MAXPLAYERS], target_count;
	bool tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_TARGET_NONE,
			target_name,
			sizeof(target_name),
			tn_is_ml)) > 0)
	{
		if (tn_is_ml)
		{
			ShowActivity2(client, "[SM] ", "%t", "Renamed target", target_name);
		}
		else
		{
			ShowActivity2(client, "[SM] ", "%t", "Renamed target", "_s", target_name);
		}

		for (int i = 0; i < target_count; i++)
		{
			if (randomize)
			{
				RandomizeName(target_list[i]);
			}
			else
			{
				if (target_count > 1)
				{
					Format(g_NewName[target_list[i]], MAX_NAME_LENGTH, "%s %i", arg2, i+1);
				}
				else
				{
					Format(g_NewName[target_list[i]], MAX_NAME_LENGTH, "%s", arg2);
				}
			}
			PerformRename(client, target_list[i]);
		}
	}
	else
	{
		ReplyToTargetError(client, target_count);
	}
	return Plugin_Handled;
}
