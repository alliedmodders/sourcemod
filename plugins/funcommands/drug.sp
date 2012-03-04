/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides drug functionality
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

new Handle:g_DrugTimers[MAXPLAYERS+1];
new Float:g_DrugAngles[20] = {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 20.0, 15.0, 10.0, 5.0, 0.0, -5.0, -10.0, -15.0, -20.0, -25.0, -20.0, -15.0, -10.0, -5.0};

CreateDrug(client)
{
	g_DrugTimers[client] = CreateTimer(1.0, Timer_Drug, client, TIMER_REPEAT);	
}

KillDrug(client)
{
	KillDrugTimer(client);
	
	new Float:angs[3];
	GetClientEyeAngles(client, angs);
	
	angs[2] = 0.0;
	
	TeleportEntity(client, NULL_VECTOR, angs, NULL_VECTOR);	
	
	new clients[2];
	clients[0] = client;	
	
	new Handle:message = StartMessageEx(g_FadeUserMsgId, clients, 1);
	BfWriteShort(message, 1536);
	BfWriteShort(message, 1536);
	BfWriteShort(message, (0x0001 | 0x0010));
	BfWriteByte(message, 0);
	BfWriteByte(message, 0);
	BfWriteByte(message, 0);
	BfWriteByte(message, 0);
	EndMessage();	
}

KillDrugTimer(client)
{
	KillTimer(g_DrugTimers[client]);
	g_DrugTimers[client] = INVALID_HANDLE;	
}

KillAllDrugs()
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if (g_DrugTimers[i] != INVALID_HANDLE)
		{
			if(IsClientInGame(i))
			{
				KillDrug(i);
			}
			else
			{
				KillDrugTimer(i);
			}
		}
	}
}

PerformDrug(client, target, toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_DrugTimers[target] == INVALID_HANDLE)
			{
				CreateDrug(target);
				LogAction(client, target, "\"%L\" drugged \"%L\"", client, target);
			}
			else
			{
				KillDrug(target);
				LogAction(client, target, "\"%L\" undrugged \"%L\"", client, target);
			}			
		}

		case (1):
		{
			if (g_DrugTimers[target] == INVALID_HANDLE)
			{
				CreateDrug(target);
				LogAction(client, target, "\"%L\" drugged \"%L\"", client, target);
			}			
		}
		
		case (0):
		{
			if (g_DrugTimers[target] != INVALID_HANDLE)
			{
				KillDrug(target);
				LogAction(client, target, "\"%L\" undrugged \"%L\"", client, target);
			}			
		}
	}
}

public Action:Timer_Drug(Handle:timer, any:client)
{
	if (!IsClientInGame(client))
	{
		KillDrugTimer(client);

		return Plugin_Handled;
	}
	
	if (!IsPlayerAlive(client))
	{
		KillDrug(client);
		
		return Plugin_Handled;
	}
	
	new Float:angs[3];
	GetClientEyeAngles(client, angs);
	
	angs[2] = g_DrugAngles[GetRandomInt(0,100) % 20];
	
	TeleportEntity(client, NULL_VECTOR, angs, NULL_VECTOR);
	
	new clients[2];
	clients[0] = client;	
	
	new Handle:message = StartMessageEx(g_FadeUserMsgId, clients, 1);
	BfWriteShort(message, 255);
	BfWriteShort(message, 255);
	BfWriteShort(message, (0x0002));
	BfWriteByte(message, GetRandomInt(0,255));
	BfWriteByte(message, GetRandomInt(0,255));
	BfWriteByte(message, GetRandomInt(0,255));
	BfWriteByte(message, 128);
	
	EndMessage();	
		
	return Plugin_Handled;
}

public AdminMenu_Drug(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Drug player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayDrugMenu(param);
	}
}

DisplayDrugMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Drug);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Drug player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_Drug(Handle:menu, MenuAction:action, param1, param2)
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
			new String:name[32];
			GetClientName(target, name, sizeof(name));
			
			PerformDrug(param1, target, 2);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled drug on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayDrugMenu(param1);
		}
	}
}

public Action:Command_Drug(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_drug <#userid|name> [0/1]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new toggle = 2;
	if (args > 1)
	{
		decl String:arg2[2];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToInt(arg2))
		{
			toggle = 1;
		}
		else
		{
			toggle = 0;
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
		PerformDrug(client, target_list[i], toggle);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled drug on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled drug on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
