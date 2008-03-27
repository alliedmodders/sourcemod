/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains menu callbacks
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

// Following callbacks are for sm_map
public AdminMenu_Map(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Choose Map", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayMenu(g_Menu_Map, param, MENU_TIME_FOREVER);
	}
}

public MenuHandler_Map(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		GetMenuItem(menu, param2, g_Client_Data[param1], sizeof(g_Client_Data[]));
		DisplayWhenMenu(param1);
	}
}

public MenuHandler_ChangeMap(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:when[2];
		GetMenuItem(menu, param2, when, sizeof(when));
		SetMapChange(client, g_Client_Data[param1], when[0])
	}
}

// Following callbacks are for sm_votemap
public AdminMenu_VoteMap(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Map vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		g_VoteMapInUse = param;
		ClearArray(g_SelectedMaps);
		DisplayMenu(g_Menu_Votemap, param, MENU_TIME_FOREVER);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running, theres no maps listed or someone else has already accessed this menu */
		buffer[0] = (!IsNewVoteAllowed() || g_VoteMapInUse) ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public MenuHandler_VoteMap(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{		
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayWhenMenu(param1, true);
		}
		else // no action was selected.
		{
			/* Re-enable the menu option */
			g_VoteMapInUse = 0;
		}
	}
	else if (action == MenuAction_DrawItem)
	{
		decl String:info[32];
		
		GetMenuItem(menu, param2, info, sizeof(info));
		
		if (FindStringInArray(g_SelectedMaps, info) != -1)
		{
			return ITEMDRAW_IGNORE;
		}
		else
		{
			return ITEMDRAW_DEFAULT;
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32];
		
		GetMenuItem(menu, param2, info, sizeof(info));
		
		PushArrayString(g_SelectedMaps, info);
		
		/* Redisplay the list */
		if (GetArraySize(g_SelectedMaps) < 5)
		{
			DisplayMenu(g_MapList, param1, MENU_TIME_FOREVER);
		}
		else
		{
			DisplayWhenMenu(param1, true);
		}
	}
	
	return 0;
}

public MenuHandler_VoteWhen(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			g_Menu_Info[param1][0] = 'i';
			DisplayConfirmVoteMenu(param1);
		}
		else
		{
			g_VoteMapInUse = 0;
		}
	}
	else if (action == MenuAction_Select)
	{
		GetMenuItem(menu, param2, g_Menu_Info[param1], sizeof(g_Menu_Info[]));
		
		DisplayConfirmVoteMenu(param1);
	}
}

public MenuHandler_Confirm(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		g_VoteMapInUse = 0;
		
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:maps[5][64];
		new selectedmaps = GetArraySize(g_SelectedMaps);
		
		for (new i = 0; i < selectedmaps; i++)
		{
			GetArrayString(g_SelectedMaps, i, maps[i], sizeof(maps[]));
		}
		
		DisplayVoteMapMenu(param1, selectedmaps, maps);
	}
}

public MenuHandler_Accept(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
		g_VoteMapInUse = 0;
	}
	else if (action == MenuAction_Select)
	{
		decl String:map[64]
		GetMenuItem(menu, 1, map, sizeof(map));
		
		SetMapChange(param1, map, g_Client_Data[param1][0])
	}
}

public Handler_MapSelectMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			if (GetArraySize(g_RTVMapList) >= GetConVarInt(g_Cvar_Maps)) 
			{
				PrintToChat(param1, "[SM] %t", "Max Nominations");
				return;	
			}
			
			decl String:map[64], String:name[64];
			GetMenuItem(menu, param2, map, sizeof(map));
			
			if (FindStringInArray(g_RTVMapList, map) != -1)
			{
				PrintToChat(param1, "[SM] %t", "Map Already Nominated");
				return;
			}
			
			GetClientName(param1, name, 64);

			PushArrayString(g_RTVMapList, map);
			RemoveMenuItem(menu, param2);
			
			g_Nominated[param1] = true;
			
			PrintToChatAll("[SM] %t", "Map Nominated", name, map);
		}
	}
}