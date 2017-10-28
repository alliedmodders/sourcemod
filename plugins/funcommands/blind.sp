/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides blind functionality
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

int g_BlindTarget[MAXPLAYERS+1];

void PerformBlind(int client, int target, int amount)
{
	int targets[2];
	targets[0] = target;
	
	int duration = 1536;
	int holdtime = 1536;
	int flags;
	if (amount == 0)
	{
		flags = (0x0001 | 0x0010);
	}
	else
	{
		flags = (0x0002 | 0x0008);
	}
	
	int color[4] = { 0, 0, 0, 0 };
	color[3] = amount;
	
	Handle message = StartMessageEx(g_FadeUserMsgId, targets, 1);
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

	LogAction(client, target, "\"%L\" set blind on \"%L\" (amount \"%d\")", client, target, amount);
}

public void AdminMenu_Blind(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Blind player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBlindMenu(param);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		// Disable if we could not find the user message id for Fade.
		buffer[0] = ((g_FadeUserMsgId == INVALID_MESSAGE_ID) ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT);
	}	
}

void DisplayBlindMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Blind);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Blind player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

void DisplayAmountMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Amount);
	
	char title[100];
	Format(title, sizeof(title), "%T: %N", "Blind amount", client, GetClientOfUserId(g_BlindTarget[client]));
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTranslatedMenuItem(menu, "255", "Fully blind", client);
	AddTranslatedMenuItem(menu, "240", "Half blind", client);
	AddTranslatedMenuItem(menu, "0", "No blind", client);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Blind(Menu menu, MenuAction action, int param1, int param2)
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
			g_BlindTarget[param1] = userid;
			DisplayAmountMenu(param1);
			return;	// Return, because we went to a new menu and don't want the re-draw to occur.
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBlindMenu(param1);
		}
	}
	
	return;
}

public int MenuHandler_Amount(Menu menu, MenuAction action, int param1, int param2)
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
		int amount, target;
		
		menu.GetItem(param2, info, sizeof(info));
		amount = StringToInt(info);

		if ((target = GetClientOfUserId(g_BlindTarget[param1])) == 0)
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
			
			PerformBlind(param1, target, amount);
			ShowActivity2(param1, "[SM] ", "%t", "Set blind on target", "_s", name, amount);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBlindMenu(param1);
		}
	}
}

public Action Command_Blind(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_blind <#userid|name> [amount]");
		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	int amount = 0;
	if (args > 1)
	{
		char arg2[20];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToIntEx(arg2, amount) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
		
		if (amount < 0)
		{
			amount = 0;
		}
		
		if (amount > 255)
		{
			amount = 255;
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
		PerformBlind(client, target_list[i], amount);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Set blind on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Set blind on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
