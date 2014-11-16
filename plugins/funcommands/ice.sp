/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides freeze and freezebomb functionality
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

new g_FreezeSerial[MAXPLAYERS+1] = { 0, ... };
new g_FreezeBombSerial[MAXPLAYERS+1] = { 0, ... };
new g_FreezeTime[MAXPLAYERS+1] = { 0, ... };
new g_FreezeBombTime[MAXPLAYERS+1] = { 0, ... };

ConVar g_Cvar_FreezeDuration;
ConVar g_Cvar_FreezeBombTicks;
ConVar g_Cvar_FreezeBombRadius;
ConVar g_Cvar_FreezeBombMode;

FreezeClient(client, time)
{
	if (g_FreezeSerial[client] != 0)
	{
		UnfreezeClient(client);
		return;
	}
	SetEntityMoveType(client, MOVETYPE_NONE);
	SetEntityRenderColor(client, 0, 128, 255, 192);

	if (g_FreezeSound[0])
	{
		new Float:vec[3];
		GetClientEyePosition(client, vec);
		EmitAmbientSound(g_FreezeSound, vec, client, SNDLEVEL_RAIDSIREN);
	}

	g_FreezeTime[client] = time;
	g_FreezeSerial[client] = ++ g_Serial_Gen;
	CreateTimer(1.0, Timer_Freeze, client | (g_Serial_Gen << 7), DEFAULT_TIMER_FLAGS);
}

UnfreezeClient(client)
{
	g_FreezeSerial[client] = 0;
	g_FreezeTime[client] = 0;

	if (IsClientInGame(client))
	{
		if (g_FreezeSound[0])
		{
			new Float:vec[3];
			GetClientAbsOrigin(client, vec);
			vec[2] += 10;	
			
			GetClientEyePosition(client, vec);
			EmitAmbientSound(g_FreezeSound, vec, client, SNDLEVEL_RAIDSIREN);
		}

		SetEntityMoveType(client, MOVETYPE_WALK);
		
		SetEntityRenderColor(client, 255, 255, 255, 255);
	}
}

CreateFreezeBomb(client)
{
	if (g_FreezeBombSerial[client] != 0)
	{
		KillFreezeBomb(client);
		return;
	}
	g_FreezeBombTime[client] = g_Cvar_FreezeBombTicks.IntValue;
	g_FreezeBombSerial[client] = ++g_Serial_Gen;
	CreateTimer(1.0, Timer_FreezeBomb, client | (g_Serial_Gen << 7), DEFAULT_TIMER_FLAGS);
}

KillFreezeBomb(client)
{
	g_FreezeBombSerial[client] = 0;
	g_FreezeBombTime[client] = 0;

	if (IsClientInGame(client))
	{
		SetEntityRenderColor(client, 255, 255, 255, 255);
	}
}

KillAllFreezes( )
{
	for(new i = 1; i <= MaxClients; i++)
	{
		if (g_FreezeSerial[i] != 0)
		{
			UnfreezeClient(i);
		}

		if (g_FreezeBombSerial[i] != 0)
		{
			KillFreezeBomb(i);
		}
	}
}

PerformFreeze(client, target, time)
{
	FreezeClient(target, time);
	LogAction(client, target, "\"%L\" froze \"%L\"", client, target);
}

PerformFreezeBomb(client, target)
{
	if (g_FreezeBombSerial[target] != 0)
	{
		KillFreezeBomb(target);
		LogAction(client, target, "\"%L\" removed a FreezeBomb on \"%L\"", client, target);
	}
	else
	{
		CreateFreezeBomb(target);
		LogAction(client, target, "\"%L\" set a FreezeBomb on \"%L\"", client, target);
	}
}

public Action:Timer_Freeze(Handle:timer, any:value)
{
	new client = value & 0x7f;
	new serial = value >> 7;

	if (!IsClientInGame(client)
		|| !IsPlayerAlive(client)
		|| g_FreezeSerial[client] != serial)
	{
		UnfreezeClient(client);
		return Plugin_Stop;
	}

	if (g_FreezeTime[client] == 0)
	{
		UnfreezeClient(client);
		
		/* HintText doesn't work on Dark Messiah */
		if (g_GameEngine != Engine_DarkMessiah)
		{
			PrintHintText(client, "%t", "Unfrozen");
		}
		else
		{
			PrintCenterText(client, "%t", "Unfrozen");
		}
		
		return Plugin_Stop;
	}

	if (g_GameEngine != Engine_DarkMessiah)
	{
		PrintHintText(client, "%t", "You will be unfrozen", g_FreezeTime[client]);
	}
	else
	{
		PrintCenterText(client, "%t", "You will be unfrozen", g_FreezeTime[client]);
	}
	
	g_FreezeTime[client]--;
	SetEntityMoveType(client, MOVETYPE_NONE);
	SetEntityRenderColor(client, 0, 128, 255, 135);

	new Float:vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 10;

	if (g_GlowSprite > -1)
	{
		TE_SetupGlowSprite(vec, g_GlowSprite, 0.95, 1.5, 50);
		TE_SendToAll();
	}
	else if (g_HaloSprite > -1)
	{
		TE_SetupGlowSprite(vec, g_HaloSprite, 0.95, 1.5, 50);
		TE_SendToAll();
	}

	return Plugin_Continue;
}

public Action:Timer_FreezeBomb(Handle:timer, any:value)
{
	new client = value & 0x7f;
	new serial = value >> 7;

	if (!IsClientInGame(client)
		|| !IsPlayerAlive(client)
		|| g_FreezeBombSerial[client] != serial)
	{
		KillFreezeBomb(client);
		return Plugin_Stop;
	}

	new Float:vec[3];
	GetClientEyePosition(client, vec);
	g_FreezeBombTime[client]--;

	if (g_FreezeBombTime[client] > 0)
	{
		new color;

		if (g_FreezeBombTime[client] > 1)
		{
			color = RoundToFloor(g_FreezeBombTime[client] * (255.0 / g_Cvar_FreezeBombTicks.FloatValue));
			if (g_BeepSound[0])
			{
				EmitAmbientSound(g_BeepSound, vec, client, SNDLEVEL_RAIDSIREN);
			}
		}
		else
		{
			color = 0;
			if (g_FinalSound[0])
			{
				EmitAmbientSound(g_FinalSound, vec, client, SNDLEVEL_RAIDSIREN);
			}
		}
		
		SetEntityRenderColor(client, color, color, 255, 255);

		char name[64];
		GetClientName(client, name, sizeof(name));
		PrintCenterTextAll("%t", "Till Explodes", name, g_FreezeBombTime[client]);

		if (g_BeamSprite > -1 && g_HaloSprite > -1)
		{
			GetClientAbsOrigin(client, vec);
			vec[2] += 10;

			TE_SetupBeamRingPoint(vec, 10.0, g_Cvar_FreezeBombRadius.FloatValue / 3.0, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
			TE_SendToAll();
			TE_SetupBeamRingPoint(vec, 10.0, g_Cvar_FreezeBombRadius.FloatValue / 3.0, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, whiteColor, 10, 0);
			TE_SendToAll();
		}
		return Plugin_Continue;
	}
	else
	{
		if (g_ExplosionSprite > -1)
		{
			TE_SetupExplosion(vec, g_ExplosionSprite, 5.0, 1, 0, g_Cvar_FreezeBombRadius.IntValue, 5000);
			TE_SendToAll();
		}

		if (g_BoomSound[0])
		{
			EmitAmbientSound(g_BoomSound, vec, client, SNDLEVEL_RAIDSIREN);
		}

		KillFreezeBomb(client);
		FreezeClient(client, g_Cvar_FreezeDuration.IntValue);
		
		if (g_Cvar_FreezeBombMode.IntValue > 0)
		{
			bool teamOnly = ((g_Cvar_FreezeBombMode.IntValue == 1) ? true : false);
			
			for (int i = 1; i <= MaxClients; i++)
			{
				if (!IsClientInGame(i) || !IsPlayerAlive(i) || i == client)
				{
					continue;
				}
				
				if (teamOnly && GetClientTeam(i) != GetClientTeam(client))
				{
					continue;
				}
				
				float pos[3];
				GetClientEyePosition(i, pos);
				
				float distance = GetVectorDistance(vec, pos);
				
				if (distance > g_Cvar_FreezeBombRadius.FloatValue)
				{
					continue;
				}
				
				if (g_HaloSprite > -1)
				{
					if (g_BeamSprite2 > -1)
					{
						TE_SetupBeamPoints(vec, pos, g_BeamSprite2, g_HaloSprite, 0, 1, 0.7, 20.0, 50.0, 1, 1.5, blueColor, 10);
						TE_SendToAll();
					}
					else if (g_BeamSprite > -1)
					{
						TE_SetupBeamPoints(vec, pos, g_BeamSprite, g_HaloSprite, 0, 1, 0.7, 20.0, 50.0, 1, 1.5, blueColor, 10);
						TE_SendToAll();
					}
				}
				
				FreezeClient(i, g_Cvar_FreezeDuration.IntValue);
			}		
		}
		return Plugin_Stop;
	}
}

public AdminMenu_Freeze(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Freeze player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayFreezeMenu(param);
	}
}

public AdminMenu_FreezeBomb(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "FreezeBomb player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayFreezeBombMenu(param);
	}
}

DisplayFreezeMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_Freeze);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Freeze player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

DisplayFreezeBombMenu(client)
{
	Menu menu = CreateMenu(MenuHandler_FreezeBomb);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "FreezeBomb player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public MenuHandler_Freeze(Menu menu, MenuAction action, int param1, int param2)
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
		else
		{
			new String:name[32];
			GetClientName(target, name, sizeof(name));
			
			PerformFreeze(param1, target, g_Cvar_FreezeDuration.IntValue);
			ShowActivity2(param1, "[SM] ", "%t", "Froze target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayFreezeMenu(param1);
		}
	}
}

public MenuHandler_FreezeBomb(Menu menu, MenuAction action, int param1, int param2)
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
		else
		{
			new String:name[32];
			GetClientName(target, name, sizeof(name));
			
			PerformFreezeBomb(param1, target);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled FreezeBomb on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayFreezeBombMenu(param1);
		}
	}
}

public Action Command_Freeze(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_freeze <#userid|name> [time]");
		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	int seconds = g_Cvar_FreezeDuration.IntValue;
	
	if (args > 1)
	{
		decl String:time[20];
		GetCmdArg(2, time, sizeof(time));
		if (StringToIntEx(time, seconds) == 0)
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
		PerformFreeze(client, target_list[i], seconds);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Froze target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Froze target", "_s", target_name);
	}
	
	return Plugin_Handled;
}

public Action:Command_FreezeBomb(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_freezebomb <#userid|name>");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

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
		PerformFreezeBomb(client, target_list[i]);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled FreezeBomb on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled FreezeBomb on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
