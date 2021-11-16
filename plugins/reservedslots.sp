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

#pragma newdecls required

public Plugin myinfo = 
{
	name = "Reserved Slots",
	author = "AlliedModders LLC",
	description = "Provides basic reserved slots",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

int g_adminCount = 0;
bool g_isAdmin[MAXPLAYERS+1];

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

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	if (GetEngineVersion() == Engine_Contagion)
	{
		// sv_visiblemaxplayers doesn't exist
		strcopy(error, err_max, "Reserved Slots is incompatible with this game");
		return APLRes_SilentFailure;
	}

	return APLRes_Success;
}

public void OnPluginStart()
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

public void OnPluginEnd()
{
	/* 	If the plugin has been unloaded, reset visiblemaxplayers. In the case of the server shutting down this effect will not be visible */
	ResetVisibleMax();
}

public void OnMapStart()
{
	CheckHiddenSlots();
}

public void OnConfigsExecuted()
{
	CheckHiddenSlots();	
}

public Action OnTimedKick(Handle timer, any client)
{	
	if (!client || !IsClientInGame(client))
	{
		return Plugin_Handled;
	}
	
	KickClient(client, "%T", "Slot reserved", client);
	
	CheckHiddenSlots();
	
	return Plugin_Handled;
}

public void OnClientPostAdminCheck(int client)
{
	int reserved = sm_reserved_slots.IntValue;

	if (reserved > 0)
	{
		int clients = GetClientCount(false);
		int limit = GetMaxHumanPlayers() - reserved;
		int flags = GetUserFlagBits(client);
		
		int type = sm_reserve_type.IntValue;
		
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
					int target = SelectKickClient();
						
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
					int target = SelectKickClient();
						
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

public void OnClientDisconnect_Post(int client)
{
	CheckHiddenSlots();
	
	if (g_isAdmin[client])
	{
		g_adminCount--;
		g_isAdmin[client] = false;	
	}
}

public void SlotCountChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	int slotcount = convar.IntValue;
	if (slotcount == 0)
	{
		ResetVisibleMax();
	}
	else if (sm_hide_slots.BoolValue)
	{
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - slotcount);
	}
}

public void SlotHideChanged(ConVar convar, const char[] oldValue, const char[] newValue)
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

void CheckHiddenSlots()
{
	if (sm_hide_slots.BoolValue)
	{		
		SetVisibleMaxSlots(GetClientCount(false), GetMaxHumanPlayers() - sm_reserved_slots.IntValue);
	}
}

void SetVisibleMaxSlots(int clients, int limit)
{
	int num = clients;
	
	if (clients == GetMaxHumanPlayers())
	{
		num = GetMaxHumanPlayers();
	} else if (clients < limit) {
		num = limit;
	}
	
	sv_visiblemaxplayers.IntValue = num;
}

void ResetVisibleMax()
{
	sv_visiblemaxplayers.IntValue = -1;
}

int SelectKickClient()
{
	KickType type = view_as<KickType>(sm_reserve_kicktype.IntValue);
	
	float highestValue;
	int highestValueId;
	
	float highestSpecValue;
	int highestSpecValueId;
	
	bool specFound;
	
	float value;
	
	for (int i=1; i<=MaxClients; i++)
	{	
		if (!IsClientConnected(i))
		{
			continue;
		}
	
		int flags = GetUserFlagBits(i);
		
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
