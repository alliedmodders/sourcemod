/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides TimeBomb functionality
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

new g_TimeBombSerial[MAXPLAYERS+1] = { 0, ... };
new g_TimeBombTime[MAXPLAYERS+1] = { 0, ... };

new Handle:g_Cvar_TimeBombTicks = INVALID_HANDLE;
new Handle:g_Cvar_TimeBombRadius = INVALID_HANDLE;
new Handle:g_Cvar_TimeBombMode = INVALID_HANDLE;

CreateTimeBomb(client)
{
	g_TimeBombSerial[client] = ++g_Serial_Gen;
	CreateTimer(1.0, Timer_TimeBomb, client | (g_Serial_Gen << 7), DEFAULT_TIMER_FLAGS);
	g_TimeBombTime[client] = GetConVarInt(g_Cvar_TimeBombTicks);
}

KillTimeBomb(client)
{
	g_TimeBombSerial[client] = 0;

	if (IsClientInGame(client))
	{
		SetEntityRenderColor(client, 255, 255, 255, 255);
	}
}

KillAllTimeBombs()
{
	for (new i = 1; i <= MaxClients; i++)
	{
		KillTimeBomb(i);
	}
}

PerformTimeBomb(client, target)
{
	if (g_TimeBombSerial[target] == 0)
	{
		CreateTimeBomb(target);
		LogAction(client, target, "\"%L\" set a TimeBomb on \"%L\"", client, target);
	}
	else
	{
		KillTimeBomb(target);
		SetEntityRenderColor(client, 255, 255, 255, 255);
		LogAction(client, target, "\"%L\" removed a TimeBomb on \"%L\"", client, target);
	}
}

public Action:Timer_TimeBomb(Handle:timer, any:value)
{
	new client = value & 0x7f;
	new serial = value >> 7;

	if (!IsClientInGame(client)
		|| !IsPlayerAlive(client)
		|| serial != g_TimeBombSerial[client])
	{
		KillTimeBomb(client);
		return Plugin_Stop;
	}	
	g_TimeBombTime[client]--;
	
	new Float:vec[3];
	GetClientEyePosition(client, vec);
	
	if (g_TimeBombTime[client] > 0)
	{
		new color;
		
		if (g_TimeBombTime[client] > 1)
		{
			color = RoundToFloor(g_TimeBombTime[client] * (128.0 / GetConVarFloat(g_Cvar_TimeBombTicks)));
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
		PrintCenterTextAll("%t", "Till Explodes", name, g_TimeBombTime[client]);
		
		GetClientAbsOrigin(client, vec);
		vec[2] += 10;

		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_Cvar_TimeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
		TE_SendToAll();
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_Cvar_TimeBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, whiteColor, 10, 0);
		TE_SendToAll();
		return Plugin_Continue;
	}
	else
	{
		if (g_ExplosionSprite > -1)
		{
			TE_SetupExplosion(vec, g_ExplosionSprite, 5.0, 1, 0, GetConVarInt(g_Cvar_TimeBombRadius), 5000);
			TE_SendToAll();
		}

		EmitAmbientSound(SOUND_BOOM, vec, client, SNDLEVEL_RAIDSIREN);

		ForcePlayerSuicide(client);
		KillTimeBomb(client);
		SetEntityRenderColor(client, 255, 255, 255, 255);
		
		if (GetConVarInt(g_Cvar_TimeBombMode) > 0)
		{
			new teamOnly = ((GetConVarInt(g_Cvar_TimeBombMode) == 1) ? true : false);
			
			for (new i = 1; i <= MaxClients; i++)
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
				
				if (distance > GetConVarFloat(g_Cvar_TimeBombRadius))
				{
					continue;
				}
				
				new damage = 220;
				damage = RoundToFloor(damage * ((GetConVarFloat(g_Cvar_TimeBombRadius) - distance) / GetConVarFloat(g_Cvar_TimeBombRadius)));
					
				SlapPlayer(i, damage, false);
				
				if (g_ExplosionSprite > -1)
				{
					TE_SetupExplosion(pos, g_ExplosionSprite, 0.05, 1, 0, 1, 1);
					TE_SendToAll();	
				}
				
				/* ToDo
				new Float:dir[3];
				SubtractVectors(vec, pos, dir);
				TR_TraceRayFilter(vec, dir, MASK_SOLID, RayType_Infinite, TR_Filter_Client);

				if (i == TR_GetEntityIndex())
				{
					new damage = 100;
					new radius = GetConVarInt(g_Cvar_TimeBombRadius) / 2;
					
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
		return Plugin_Stop;
	}
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
			
			PerformTimeBomb(param1, target);
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
		ReplyToCommand(client, "[SM] Usage: sm_timebomb <#userid|name>");
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
		PerformTimeBomb(client, target_list[i]);
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
