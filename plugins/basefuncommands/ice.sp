/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides freeze and freezebomb functionality
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

new Handle:g_FreezeTimers[MAXPLAYERS+1];
new Handle:g_FreezeBombTimers[MAXPLAYERS+1];
new g_FreezeTracker[MAXPLAYERS+1];
new g_FreezeBombTracker[MAXPLAYERS+1];

new Handle:g_FreezeDuration = INVALID_HANDLE;
new Handle:g_FreezeBombTicks = INVALID_HANDLE;
new Handle:g_FreezeBombRadius = INVALID_HANDLE;
new Handle:g_FreezeBombMode = INVALID_HANDLE;

SetupIce()
{
	RegAdminCmd("sm_freeze", Command_Freeze, ADMFLAG_SLAY, "sm_freeze <#userid|name> [time]");
	RegAdminCmd("sm_freezebomb", Command_FreezeBomb, ADMFLAG_SLAY, "sm_freezebomb <#userid|name> [0/1]");

	g_FreezeDuration = CreateConVar("sm_freeze_duration", "10.0", "Sets the default duration for sm_freeze and freezebomb victims", 0, true, 1.0, true, 120.0);	
	g_FreezeBombTicks = CreateConVar("sm_freezebomb_ticks", "10", "Sets how long the freezebomb fuse is.", 0, true, 5.0, true, 120.0);
	g_FreezeBombRadius = CreateConVar("sm_freezebomb_radius", "600", "Sets the freezebomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_FreezeBombMode = CreateConVar("sm_freezebomb_mode", "0", "Who is targetted by the freezebomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
}

FreezeClient(client, time)
{
	if (g_FreezeTimers[client] != INVALID_HANDLE)
	{
		UnfreezeClient(client);
	}
	
	SetEntityMovetype(client, MOVETYPE_NONE);
	SetEntityRenderColor(client, 0, 128, 255, 192);
	
	new Float:vec[3];
	GetClientEyePosition(client, vec);
	EmitAmbientSound(SOUND_FREEZE, vec, client, SNDLEVEL_RAIDSIREN);

	g_FreezeTimers[client] = CreateTimer(1.0, Timer_Freeze, client, TIMER_REPEAT);
	g_FreezeTracker[client] = time;
}

UnfreezeClient(client)
{
	KillFreezeTimer(client);

	new Float:vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 10;	
	
	GetClientEyePosition(client, vec);
	EmitAmbientSound(SOUND_FREEZE, vec, client, SNDLEVEL_RAIDSIREN);

	SetEntityMovetype(client, MOVETYPE_WALK);
	SetEntityRenderColor(client, 255, 255, 255, 255);	
}

KillFreezeTimer(client)
{
	KillTimer(g_FreezeTimers[client]);
	g_FreezeTimers[client] = INVALID_HANDLE;
}

CreateFreezeBomb(client)
{
	g_FreezeBombTimers[client] = CreateTimer(1.0, Timer_FreezeBomb, client, TIMER_REPEAT);
	g_FreezeBombTracker[client] = GetConVarInt(g_FreezeBombTicks);
}

KillFreezeBomb(client)
{
	KillTimer(g_FreezeBombTimers[client]);
	g_FreezeBombTimers[client] = INVALID_HANDLE;
	
	SetEntityRenderColor(client, 255, 255, 255, 255);
}

KillAllFreezes()
{
	new maxclients = GetMaxClients();
	for (new i = 1; i <= maxclients; i++)
	{
		if (g_FreezeTimers[i] != INVALID_HANDLE)
		{
			if(IsClientInGame(i))
			{
				UnfreezeClient(i);
			}
			else
			{
				KillFreezeTimer(i);
			}			
		}		
		
		if (g_FreezeBombTimers[i] != INVALID_HANDLE)
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

PerformFreezeBomb(client, target, toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_FreezeBombTimers[target] == INVALID_HANDLE)
			{
				CreateFreezeBomb(target);
				LogAction(client, target, "\"%L\" set a FreezeBomb on \"%L\"", client, target);
			}
			else
			{
				KillFreezeBomb(target);
				LogAction(client, target, "\"%L\" removed a FreezeBomb on \"%L\"", client, target);
			}
		}

		case (1):
		{
			if (g_FreezeBombTimers[target] == INVALID_HANDLE)
			{
				CreateFreezeBomb(target);
				LogAction(client, target, "\"%L\" set a FreezeBomb on \"%L\"", client, target);
			}
		}
		
		case (0):
		{
			if (g_FreezeBombTimers[target] != INVALID_HANDLE)
			{
				KillFreezeBomb(target);
				LogAction(client, target, "\"%L\" removed a FreezeBomb on \"%L\"", client, target);
			}
		}
	}
}

public Action:Timer_Freeze(Handle:timer, any:client)
{
	if (!IsClientInGame(client))
	{
		KillFreezeTimer(client);

		return Plugin_Handled;
	}
	
	if (!IsPlayerAlive(client))
	{
		UnfreezeClient(client);
		
		return Plugin_Handled;
	}		
	
	g_FreezeTracker[client]--;
	
	SetEntityMovetype(client, MOVETYPE_NONE);
	SetEntityRenderColor(client, 0, 128, 255, 135);
	
	new Float:vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 10;
	
	TE_SetupGlowSprite(vec, g_GlowSprite, 0.95, 1.5, 50);
	TE_SendToAll();	

	if (g_FreezeTracker[client] == 0)
	{
		UnfreezeClient(client);
	}

	return Plugin_Handled;
}

public Action:Timer_FreezeBomb(Handle:timer, any:client)
{
	if (!IsClientInGame(client) || !IsPlayerAlive(client))
	{
		KillFreezeBomb(client);

		return Plugin_Handled;
	}
	
	g_FreezeBombTracker[client]--;
	
	new Float:vec[3];
	GetClientEyePosition(client, vec);
	
	if (g_FreezeBombTracker[client] > 0)
	{
		new color;
		
		if (g_FreezeBombTracker[client] > 1)
		{
			color = RoundToFloor(g_FreezeBombTracker[client] * (255.0 / GetConVarFloat(g_FreezeBombTicks)));
			EmitAmbientSound(SOUND_BEEP, vec, client, SNDLEVEL_RAIDSIREN);	
		}
		else
		{
			color = 0;
			EmitAmbientSound(SOUND_FINAL, vec, client, SNDLEVEL_RAIDSIREN);
		}
		
		SetEntityRenderColor(client, color, color, 255, 255);

		decl String:name[64];
		GetClientName(client, name, sizeof(name));
		PrintCenterTextAll("%t", "Till Explodes", name, g_FreezeBombTracker[client]);
		
		GetClientAbsOrigin(client, vec);
		vec[2] += 10;

		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_FreezeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
		TE_SendToAll();
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_FreezeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, whiteColor, 10, 0);
		TE_SendToAll();
	}
	else
	{
		TE_SetupExplosion(vec, g_ExplosionSprite, 5.0, 1, 0, GetConVarInt(g_FreezeBombRadius), 5000);
		TE_SendToAll();

		EmitAmbientSound(SOUND_BOOM, vec, client, SNDLEVEL_RAIDSIREN);

		KillFreezeBomb(client);
		FreezeClient(client, GetConVarInt(g_FreezeDuration));
		
		if (GetConVarInt(g_FreezeBombMode) > 0)
		{
			new teamOnly = ((GetConVarInt(g_FreezeBombMode) == 1) ? true : false);
			new maxClients = GetMaxClients();
			
			for (new i = 1; i < maxClients; i++)
			{
				if (!IsClientInGame(i) || !IsPlayerAlive(i) || i == client)
				{
					continue;
				}
				
				if (teamOnly && GetClientTeam(i) != GetClientTeam(client))
				{
					continue;
				}
				
				new Float:pos[3];
				GetClientEyePosition(i, pos);
				
				new Float:distance = GetVectorDistance(vec, pos);
				
				if (distance > GetConVarFloat(g_FreezeBombRadius))
				{
					continue;
				}
				
				TE_SetupBeamPoints(vec, pos, g_BeamSprite2, g_HaloSprite, 0, 1, 0.7, 20.0, 50.0, 1, 1.5, blueColor, 10);
				TE_SendToAll();
				
				FreezeClient(i, GetConVarInt(g_FreezeDuration));
			}		
		}
	}
			
	return Plugin_Handled;
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
	new Handle:menu = CreateMenu(MenuHandler_Freeze);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Freeze player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayFreezeBombMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_FreezeBomb);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "FreezeBomb player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_Freeze(Handle:menu, MenuAction:action, param1, param2)
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
			
			PerformFreeze(param1, target, GetConVarInt(g_FreezeDuration));
			ShowActivity2(param1, "[SM] ", "%t", "Froze target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayFreezeMenu(param1);
		}
	}
}

public MenuHandler_FreezeBomb(Handle:menu, MenuAction:action, param1, param2)
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
			
			PerformFreezeBomb(param1, target, 2);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled FreezeBomb on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayFreezeBombMenu(param1);
		}
	}
}

public Action:Command_Freeze(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_freeze <#userid|name> [time]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new seconds = GetConVarInt(g_FreezeDuration);
	
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
		ReplyToCommand(client, "[SM] Usage: sm_freezebomb <#userid|name> [0/1]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new toggle = 2;
	if (args > 1)
	{
		decl String:arg2[2];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (arg2[0])
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
		PerformFreezeBomb(client, target_list[i], toggle);
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
