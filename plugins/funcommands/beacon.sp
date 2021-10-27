/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides beacon functionality
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

int g_BeaconSerial[MAXPLAYERS+1] = { 0, ... };

ConVar g_Cvar_BeaconRadius;

void CreateBeacon(int client)
{
	g_BeaconSerial[client] = ++g_Serial_Gen;
	CreateTimer(1.0, Timer_Beacon, client | (g_Serial_Gen << 7), DEFAULT_TIMER_FLAGS);	
}

void KillBeacon(int client)
{
	g_BeaconSerial[client] = 0;

	if (IsClientInGame(client))
	{
		SetEntityRenderColor(client, 255, 255, 255, 255);
	}
}

void KillAllBeacons()
{
	for (int i = 1; i <= MaxClients; i++)
	{
		KillBeacon(i);
	}
}

void PerformBeacon(int client, int target)
{
	if (g_BeaconSerial[target] == 0)
	{
		CreateBeacon(target);
		LogAction(client, target, "\"%L\" set a beacon on \"%L\"", client, target);
	}
	else
	{
		KillBeacon(target);
		LogAction(client, target, "\"%L\" removed a beacon on \"%L\"", client, target);
	}
}

public Action Timer_Beacon(Handle timer, any value)
{
	int client = value & 0x7f;
	int serial = value >> 7;

	if (!IsClientInGame(client)
		|| !IsPlayerAlive(client)
		|| g_BeaconSerial[client] != serial)
	{
		KillBeacon(client);
		return Plugin_Stop;
	}

	float vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 10;
	
	if (g_BeamSprite > -1 && g_HaloSprite > -1)
	{
		int teamBeaconColor[4];

		switch (GetClientTeam(client))
		{
			case 1: teamBeaconColor = g_Team1BeaconColor;
			case 2: teamBeaconColor = g_Team2BeaconColor;
			case 3: teamBeaconColor = g_Team3BeaconColor;
			case 4: teamBeaconColor = g_Team4BeaconColor;
			default: teamBeaconColor = g_TeamUnknownBeaconColor;
		}

		TE_SetupBeamRingPoint(vec, 10.0, g_Cvar_BeaconRadius.FloatValue, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, g_ExternalBeaconColor, 10, 0);
		TE_SendToAll();

		TE_SetupBeamRingPoint(vec, 10.0, g_Cvar_BeaconRadius.FloatValue, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, teamBeaconColor, 10, 0);
		TE_SendToAll();
	}
	
	if (g_BlipSound[0])
	{
		GetClientEyePosition(client, vec);
		EmitAmbientSound(g_BlipSound, vec, client, SNDLEVEL_RAIDSIREN);	
	}
		
	return Plugin_Continue;
}

public void AdminMenu_Beacon(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Beacon player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBeaconMenu(param);
	}
}

void DisplayBeaconMenu(int client)
{
	Menu menu = new Menu(MenuHandler_Beacon);
	
	char title[100];
	Format(title, sizeof(title), "%T:", "Beacon player", client);
	menu.SetTitle(title);
	menu.ExitBackButton = true;
	
	AddTargetsToMenu(menu, client, true, true);
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Beacon(Menu menu, MenuAction action, int param1, int param2)
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
			
			PerformBeacon(param1, target);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled beacon on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBeaconMenu(param1);
		}
	}
	
	return 0;
}

public Action Command_Beacon(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_beacon <#userid|name>");
		return Plugin_Handled;
	}

	char arg[65];
	GetCmdArg(1, arg, sizeof(arg));

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
		PerformBeacon(client, target_list[i]);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
