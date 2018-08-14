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

Handle g_DrugTimers[MAXPLAYERS+1];
float g_DrugAngles[20] = {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 20.0, 15.0, 10.0, 5.0, 0.0, -5.0, -10.0, -15.0, -20.0, -25.0, -20.0, -15.0, -10.0, -5.0};

void CreateDrug(int client)
{
	g_DrugTimers[client] = CreateTimer(1.0, Timer_Drug, client, TIMER_REPEAT);	
}

void KillDrug(int client)
{
	KillDrugTimer(client);
	
	float angs[3];
	GetClientEyeAngles(client, angs);
	
	angs[2] = 0.0;
	
	TeleportEntity(client, NULL_VECTOR, angs, NULL_VECTOR);	
	
	int clients[2];
	clients[0] = client;

	int duration = 1536;
	int holdtime = 1536;
	int flags = (0x0001 | 0x0010);
	int color[4] = { 0, 0, 0, 0 };

	Handle message = StartMessageEx(g_FadeUserMsgId, clients, 1);
	if (GetUserMessageType() == UM_Protobuf)
	{
		Protobuf pb = UserMessageToProtobuf(message);
		pb.SetInt("duration", duration);
		pb.SetInt("hold_time", holdtime);
		pb.SetInt("flags", flags);
		pb.SetColor("clr", color);
	}
	else
	{	
		BfWrite bf = UserMessageToBfWrite(message);
		bf.WriteShort(duration);
		bf.WriteShort(holdtime);
		bf.WriteShort(flags);
		bf.WriteByte(color[0]);
		bf.WriteByte(color[1]);
		bf.WriteByte(color[2]);
		bf.WriteByte(color[3]);
	}
	
	EndMessage();
}

void KillDrugTimer(int client)
{
	KillTimer(g_DrugTimers[client]);
	g_DrugTimers[client] = null;	
}

void KillAllDrugs()
{
	for (int i = 1; i <= MaxClients; i++)
	{
		if (g_DrugTimers[i] != null)
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

void PerformDrug(int client, int target, int toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_DrugTimers[target] == null)
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
			if (g_DrugTimers[target] == null)
			{
				CreateDrug(target);
				LogAction(client, target, "\"%L\" drugged \"%L\"", client, target);
			}			
		}
		
		case (0):
		{
			if (g_DrugTimers[target] != null)
			{
				KillDrug(target);
				LogAction(client, target, "\"%L\" undrugged \"%L\"", client, target);
			}			
		}
	}
}

public Action Timer_Drug(Handle timer, any client)
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
	
	float angs[3];
	GetClientEyeAngles(client, angs);
	
	angs[2] = g_DrugAngles[GetRandomInt(0,100) % 20];
	
	TeleportEntity(client, NULL_VECTOR, angs, NULL_VECTOR);
	
	int clients[2];
	clients[0] = client;	
	
	int duration = 255;
	int holdtime = 255;
	int flags = 0x0002;
	int color[4] = { 0, 0, 0, 128 };
	color[0] = GetRandomInt(0,255);
	color[1] = GetRandomInt(0,255);
	color[2] = GetRandomInt(0,255);

	Handle message = StartMessageEx(g_FadeUserMsgId, clients, 1);
	if (GetUserMessageType() == UM_Protobuf)
	{
		Protobuf pb = UserMessageToProtobuf(message);
		pb.SetInt("duration", duration);
		pb.SetInt("hold_time", holdtime);
		pb.SetInt("flags", flags);
		pb.SetColor("clr", color);
	}
	else
	{
		BfWriteShort(message, duration);
		BfWriteShort(message, holdtime);
		BfWriteShort(message, flags);
		BfWriteByte(message, color[0]);
		BfWriteByte(message, color[1]);
		BfWriteByte(message, color[2]);
		BfWriteByte(message, color[3]);
	}
	
	EndMessage();
		
	return Plugin_Handled;
}

public void AdminMenu_Drug(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
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

void DisplayDrugMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Drug);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Drug player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Drug(Menu menu, MenuAction action, int param1, int param2)
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

public Action Command_Drug(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_drug <#userid|name> [0/1]");
		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	int toggle = 2;
	if (args > 1)
	{
		char arg2[2];
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
