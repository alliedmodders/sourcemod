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
new Handle:sm_reserved_slots;
new Handle:sm_hide_slots;
new Handle:sv_visiblemaxplayers;
new Handle:sm_reserve_type;
new Handle:sm_reserve_maxadmins;
new Handle:sm_reserve_kicktype;

new g_SDKVersion;
new g_SourceTV = -1;
new g_Replay = -1;

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
	
	HookConVarChange(sm_reserved_slots, SlotCountChanged);
	HookConVarChange(sm_hide_slots, SlotHideChanged);
	
	g_SDKVersion = GuessSDKVersion();
	
	if (g_SDKVersion == SOURCE_SDK_EPISODE2VALVE)
	{
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!IsClientConnected(i))
				continue;
			
			if (IsClientSourceTV(i))
			{
				g_SourceTV = i;
			}
			else if (IsClientReplay(i))
			{
				g_Replay = i;
			}
		}
	}
}

public OnPluginEnd()
{
	/* 	If the plugin has been unloaded, reset visiblemaxplayers. In the case of the server shutting down this effect will not be visible */
	ResetVisibleMax();
}

public OnMapStart()
{
	if (GetConVarBool(sm_hide_slots))
	{		
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - GetConVarInt(sm_reserved_slots));
	}
}

public OnConfigsExecuted()
{
	if (GetConVarBool(sm_hide_slots))
	{
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - GetConVarInt(sm_reserved_slots));
	}	
}

public Action:OnTimedKick(Handle:timer, any:client)
{	
	if (!client || !IsClientInGame(client))
	{
		return Plugin_Handled;
	}
	
	KickClient(client, "%T", "Slot reserved", client);
	
	if (GetConVarBool(sm_hide_slots))
	{				
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - GetConVarInt(sm_reserved_slots));
	}
	
	return Plugin_Handled;
}

public OnClientPostAdminCheck(client)
{
	if (g_SDKVersion == SOURCE_SDK_EPISODE2VALVE)
	{
		if (IsClientSourceTV(client))
		{
			g_SourceTV = client;
		}
		else if (IsClientReplay(client))
		{
			g_Replay = client;
		}
	}
	
	new reserved = GetConVarInt(sm_reserved_slots);

	if (reserved > 0)
	{
		new clients = GetClientCount(false);
		new limit = MaxClients - reserved;
		new flags = GetUserFlagBits(client);
		
		new type = GetConVarInt(sm_reserve_type);
		
		if (type == 0)
		{
			if (clients <= limit || IsFakeClient(client) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
			{
				if (GetConVarBool(sm_hide_slots))
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
			
			if (clients > limit && g_adminCount < GetConVarInt(sm_reserve_maxadmins))
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
	if (g_SDKVersion == SOURCE_SDK_EPISODE2VALVE)
	{
		if (client == g_SourceTV)
		{
			g_SourceTV = -1;
		}
		else if (client == g_Replay)
		{
			g_Replay = -1;
		}
	}
	
	if (GetConVarBool(sm_hide_slots))
	{		
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - GetConVarInt(sm_reserved_slots));
	}
	
	if (g_isAdmin[client])
	{
		g_adminCount--;
		g_isAdmin[client] = false;	
	}
}

public SlotCountChanged(Handle:convar, const String:oldValue[], const String:newValue[])
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	new slotcount = GetConVarInt(convar);
	if (slotcount == 0)
	{
		ResetVisibleMax();
	}
	else if (GetConVarBool(sm_hide_slots))
	{
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - slotcount);
	}
}

public SlotHideChanged(Handle:convar, const String:oldValue[], const String:newValue[])
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	if (!GetConVarBool(convar))
	{
		ResetVisibleMax();
	}
	else
	{
		SetVisibleMaxSlots(GetClientCount(false), MaxClients - GetConVarInt(sm_reserved_slots));
	}
}

SetVisibleMaxSlots(clients, limit)
{
	new num = clients;
	
	if (clients == MaxClients)
	{
		num = MaxClients;
	} else if (clients < limit) {
		num = limit;
	}
	
	if (g_SDKVersion == SOURCE_SDK_EPISODE2VALVE)
	{
		if (g_SourceTV > -1)
		{
			--num;
		}
		
		if (g_Replay > -1)
		{
			--num;
		}
	}
	
	SetConVarInt(sv_visiblemaxplayers, num);
}

ResetVisibleMax()
{
	SetConVarInt(sv_visiblemaxplayers, -1);
}

SelectKickClient()
{
	new KickType:type = KickType:GetConVarInt(sm_reserve_kicktype);
	
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
		
		if (IsFakeClient(i) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION || CheckCommandAccess(i, "sm_reskick_immunity", ADMFLAG_RESERVATION, false))
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