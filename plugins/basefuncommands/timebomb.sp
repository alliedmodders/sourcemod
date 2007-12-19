/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides TimeBomb functionality
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

new Handle:g_TimeBombTimers[MAXPLAYERS+1];
new g_TimeBombTracker[MAXPLAYERS+1];

new Handle:g_TimeBombTicks = INVALID_HANDLE;
new Handle:g_TimeBombRadius = INVALID_HANDLE;
new Handle:g_TimeBombMode = INVALID_HANDLE;

SetupTimeBomb()
{
	RegAdminCmd("sm_timebomb", Command_TimeBomb, ADMFLAG_SLAY, "sm_timebomb <#userid|name> [0/1]");	
	
	g_TimeBombTicks = CreateConVar("sm_timebomb_ticks", "10", "Sets how long the timebomb fuse is.", 0, true, 5.0, true, 120.0);
	g_TimeBombRadius = CreateConVar("sm_timebomb_radius", "600", "Sets the bomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_TimeBombMode = CreateConVar("sm_timebomb_mode", "0", "Who is killed by the timebomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
}

CreateTimeBomb(client)
{
	g_TimeBombTimers[client] = CreateTimer(1.0, Timer_TimeBomb, client, TIMER_REPEAT);
	g_TimeBombTracker[client] = GetConVarInt(g_TimeBombTicks);
}

KillTimeBomb(client)
{
	KillTimer(g_TimeBombTimers[client]);
	g_TimeBombTimers[client] = INVALID_HANDLE;
	
	SetEntityRenderColor(client, 255, 255, 255, 255);
}

KillAllTimeBombs()
{
	new maxclients = GetMaxClients();
	for (new i = 1; i <= maxclients; i++)
	{
		if (g_TimeBombTimers[i] != INVALID_HANDLE)
		{
			KillTimeBomb(i);
		}
	}
}

PerformTimeBomb(client, target, toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_TimeBombTimers[target] == INVALID_HANDLE)
			{
				CreateTimeBomb(target);
				LogAction(client, target, "\"%L\" set a TimeBomb on \"%L\"", client, target);
			}
			else
			{
				KillTimeBomb(target);
				LogAction(client, target, "\"%L\" removed a TimeBomb on \"%L\"", client, target);
			}			
		}

		case (1):
		{
			if (g_TimeBombTimers[target] == INVALID_HANDLE)
			{
				CreateTimeBomb(target);
				LogAction(client, target, "\"%L\" set a TimeBomb on \"%L\"", client, target);
			}			
		}
		
		case (0):
		{
			if (g_TimeBombTimers[target] != INVALID_HANDLE)
			{
				KillTimeBomb(target);
				LogAction(client, target, "\"%L\" removed a TimeBomb on \"%L\"", client, target);
			}			
		}
	}
}

public Action:Timer_TimeBomb(Handle:timer, any:client)
{
	if (!IsClientInGame(client) || !IsPlayerAlive(client))
	{
		KillTimeBomb(client);		

		return Plugin_Handled;
	}
	
	g_TimeBombTracker[client]--;
	
	new Float:vec[3];
	GetClientEyePosition(client, vec);
	
	if (g_TimeBombTracker[client] > 1)
	{
		new color;
		
		if (g_TimeBombTracker[client] > 1)
		{
			color = RoundToFloor(g_TimeBombTracker[client] * (128.0 / GetConVarFloat(g_TimeBombTicks)));
			EmitAmbientSound(SOUND_BEEP, vec, client, SNDLEVEL_RAIDSIREN);	
		}
		else
		{
			color = 0;
			EmitAmbientSound(SOUND_FINAL, vec, client, SNDLEVEL_RAIDSIREN);
		}
		
		SetEntityRenderColor(client, 255, 128, color, 255);

		decl String:name[64];
		GetClientName(client, name, sizeof(name));
		PrintCenterTextAll("%t", "Till Explodes", name, g_TimeBombTracker[client]);
		
		GetClientAbsOrigin(client, vec);
		vec[2] += 10;

		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_TimeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
		TE_SendToAll();
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_TimeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, whiteColor, 10, 0);
		TE_SendToAll();
	}
	else
	{
		TE_SetupExplosion(vec, g_ExplosionSprite, 5.0, 1, 0, GetConVarInt(g_TimeBombRadius), 5000);
		TE_SendToAll();

		EmitAmbientSound(SOUND_BOOM, vec, client, SNDLEVEL_RAIDSIREN);

		ForcePlayerSuicide(client);
		KillTimeBomb(client);
		
		if (GetConVarInt(g_TimeBombMode) > 0)
		{
			new teamOnly = ((GetConVarInt(g_TimeBombMode) == 1) ? true : false);
			new maxClients = GetMaxClients();
			
			//g_FilteredEntity = client;
			
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
				
				if (distance > GetConVarFloat(g_TimeBombRadius))
				{
					continue;
				}
				
				new damage = 220;
				damage = RoundToFloor(damage * ((GetConVarFloat(g_TimeBombRadius) - distance) / GetConVarFloat(g_TimeBombRadius)));
					
				SlapPlayer(i, damage, false);
				TE_SetupExplosion(pos, g_ExplosionSprite, 0.05, 1, 0, 1, 1);
				TE_SendToAll();				
				
				/*
				new Float:dir[3];
				SubtractVectors(vec, pos, dir);
				TR_TraceRayFilter(vec, dir, MASK_SOLID, RayType_Infinite, TR_Filter_Client);

				if (i == TR_GetEntityIndex())
				{
					new damage = 100;
					new radius = GetConVarInt(g_TimeBombRadius) / 2;
					
					if (distance > radius)
					{
						distance -= radius;
						damage = RoundToFloor(damage * ((radius - distance) / radius));
					}
					
					SlapPlayer(i, damage, false);
				}
				*/
			}		
		}
	}
			
	return Plugin_Handled;
}

public AdminMenu_TimeBomb(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "TimeBomb player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayTimeBombMenu(param);
	}
}

DisplayTimeBombMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_TimeBomb);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "TimeBomb player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_TimeBomb(Handle:menu, MenuAction:action, param1, param2)
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
			
			PerformTimeBomb(param1, target, 2);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled TimeBomb on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayTimeBombMenu(param1);
		}
	}
}

public Action:Command_TimeBomb(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_timebomb <#userid|name> [0/1]");
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
		PerformTimeBomb(client, target_list[i], toggle);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled TimeBomb on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled TimeBomb on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
