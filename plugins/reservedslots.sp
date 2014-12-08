/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Reserved Slots Plugin
 * Provides basic reserved slots.
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

#pragma semicolon 1

#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Reserved Slots",
	author = "AlliedModders LLC",
	description = "Provides basic reserved slots",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new g_adminCount = 0;
new bool:g_isAdmin[MAXPLAYERS+1];

/* Handles to convars used by plugin */
ConVar sm_reserved_slots;
ConVar sm_hide_slots;
ConVar sv_visiblemaxplayers;
ConVar sm_reserve_type;
ConVar sm_reserve_maxadmins;
ConVar sm_reserve_kicktype;

enum KickType
{
	Kick_HighestPing,
	Kick_HighestTime,
	Kick_Random,	
};

public OnPluginStart()
{
	LoadTranslations("reservedslots.phrases");
	
	sm_reserved_slots = CreateConVar("sm_reserved_slots", "0", "Number of reserved player slots", 0, true, 0.0);
	sm_hide_slots = CreateConVar("sm_hide_slots", "0", "If set to 1, reserved slots will hidden (subtracted from the max slot count)", 0, true, 0.0, true, 1.0);
	sv_visiblemaxplayers = FindConVar("sv_visiblemaxplayers");
	sm_reserve_type = CreateConVar("sm_reserve_type", "0", "Method of reserving slots", 0, true, 0.0, true, 2.0);
	sm_reserve_maxadmins = CreateConVar("sm_reserve_maxadmins", "1", "Maximum amount of admins to let in the server with reserve type 2", 0, true, 0.0);
	sm_reserve_kicktype = CreateConVar("sm_reserve_kicktype", "0", "How to select a client to kick (if appropriate)", 0, true, 0.0, true, 2.0);
	
	sm_reserved_slots.AddChangeHook(SlotCountChanged);
	sm_hide_slots.AddChangeHook(SlotHideChanged);
}

public OnPluginEnd()
{
	/* 	If the plugin has been unloaded, reset visiblemaxplayers. In the case of the server shutting down this effect will not be visible */
	ResetVisibleMax();
}

public OnMapStart()
{
	if (sm_hide_slots.BoolValue)
	{
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}
}

public OnConfigsExecuted()
{
	if (sm_hide_slots.BoolValue)
	{
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}	
}

public Action:OnTimedKick(Handle:timer, any:client)
{	
	if (!client || !IsClientInGame(client))
	{
		return Plugin_Handled;
	}
	
	KickClient(client, "%T", "Slot reserved", client);
	
	if (sm_hide_slots.BoolValue)
	{				
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}
	
	return Plugin_Handled;
}

public OnClientPostAdminCheck(client)
{
	new reserved = sm_reserved_slots.IntValue;

	if (reserved > 0)
	{
		new clients = GetClientCount(false);
		new limit = GetMaxHumanPlayers() - reserved;
		new flags = GetUserFlagBits(client);
		
		new type = sm_reserve_type.IntValue;
		
		if (type == 0)
		{
			if (clients <= limit || IsFakeClient(client) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
			{
				if (sm_hide_slots.BoolValue)
				{
					SetVisibleMaxSlots(clients, limit);
				}
				
				return;
			}
			
			/* Kick player because there are no public slots left */
			CreateTimer(0.1, OnTimedKick, client);
		}
		else if (type == 1)
		{	
			if (clients > limit)
			{
				if (flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
				{
					new target = SelectKickClient();
						
					if (target)
					{
						/* Kick public player to free the reserved slot again */
						CreateTimer(0.1, OnTimedKick, target);
					}
				}
				else
				{				
					/* Kick player because there are no public slots left */
					CreateTimer(0.1, OnTimedKick, client);
				}
			}
		}
		else if (type == 2)
		{
			if (flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
			{
				g_adminCount++;
				g_isAdmin[client] = true;
			}
			
			if (clients > limit && g_adminCount < sm_reserve_maxadmins.IntValue)
			{
				/* Server is full, reserved slots aren't and client doesn't have reserved slots access */
				
				if (g_isAdmin[client])
				{
					new target = SelectKickClient();
						
					if (target)
					{
						/* Kick public player to free the reserved slot again */
						CreateTimer(0.1, OnTimedKick, target);
					}
				}
				else
				{				
					/* Kick player because there are no public slots left */
					CreateTimer(0.1, OnTimedKick, client);
				}		
			}
		}
	}
}

public OnClientDisconnect_Post(client)
{
	if (sm_hide_slots.BoolValue)
	{		
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}
	
	if (g_isAdmin[client])
	{
		g_adminCount--;
		g_isAdmin[client] = false;	
	}
}

public SlotCountChanged(ConVar convar, const String:oldValue[], const String:newValue[])
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	new slotcount = convar.IntValue;
	if (slotcount == 0)
	{
		ResetVisibleMax();
	}
	else if (sm_hide_slots.BoolValue)
	{
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - slotcount);
	}
}

public SlotHideChanged(ConVar convar, const String:oldValue[], const String:newValue[])
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	if (!convar.BoolValue)
	{
		ResetVisibleMax();
	}
	else
	{
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}
}

SetVisibleMaxSlots(clients, limit)
{
	new num = clients;
	
	if (clients == GetMaxHumanPlayers())
	{
		num = GetMaxHumanPlayers();
	} else if (clients < limit) {
		num = limit;
	}
	
	sv_visiblemaxplayers.IntValue = num;
}

ResetVisibleMax()
{
	sv_visiblemaxplayers.IntValue = -1;
}

SelectKickClient()
{
	new KickType:type = KickType:sm_reserve_kicktype.IntValue;
	
	new Float:highestValue;
	new highestValueId;
	
	new Float:highestSpecValue;
	new highestSpecValueId;
	
	new bool:specFound;
	
	new Float:value;
	
	for (new i=1; i<=MaxClients; i++)
	{	
		if (!IsClientConnected(i))
		{
			continue;
		}
	
		new flags = GetUserFlagBits(i);
		
		if (IsFakeClient(i) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION || CheckCommandAccess(i, "sm_reskick_immunity", ADMFLAG_RESERVATION, true))
		{
			continue;
		}
		
		value = 0.0;
			
		if (IsClientInGame(i))
		{
			if (type == Kick_HighestPing)
			{
				value = GetClientAvgLatency(i, NetFlow_Outgoing);
			}
			else if (type == Kick_HighestTime)
			{
				value = GetClientTime(i);
			}
			else
			{
				value = GetRandomFloat(0.0, 100.0);
			}

			if (IsClientObserver(i))
			{			
				specFound = true;
				
				if (value > highestSpecValue)
				{
					highestSpecValue = value;
					highestSpecValueId = i;
				}
			}
		}
		
		if (value >= highestValue)
		{
			highestValue = value;
			highestValueId = i;
		}
	}
	
	if (specFound)
	{
		return highestSpecValueId;
	}
	
	return highestValueId;
}
