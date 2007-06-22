/**
 * reservedslots.sp
 * Provides basic reserved slots.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

/* Maximum number of clients that can connect to server */
new g_MaxClients;

/* Handles to convars used by plugin */
new Handle:sm_reserved_slots;
new Handle:sm_hide_slots;
new Handle:sv_visiblemaxplayers;

public OnPluginStart()
{
	LoadTranslations("plugin.reservedslots.cfg");
	
	sm_reserved_slots = CreateConVar("sm_reserved_slots", "0", "Number of reserved player slots", 0, true, 0.0);
	sm_hide_slots = CreateConVar("sm_hide_slots", "0", "If set to 1, reserved slots will hidden (subtracted from the max slot count)", 0, true, 0.0, true, 1.0);
	sv_visiblemaxplayers = FindConVar("sv_visiblemaxplayers");
}

public OnMapStart()
{
	g_MaxClients = GetMaxClients();
}

public OnConfigsExecuted()
{
	if (GetConVarBool(sm_hide_slots))
	{
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}	
}

public OnClientAuthorized(client, const String:auth[])
{
	new reserved = GetConVarInt(sm_reserved_slots);
	
	if (reserved > 0)
	{
		new clients = GetClientCount(false);
		new limit = g_MaxClients - reserved;
		new flags = GetUserFlagBits(client);
		
		if (clients <= limit || IsFakeClient(client) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
		{
			if (GetConVarBool(sm_hide_slots))
			{
				SetVisibleMaxSlots(clients, limit);
			}
			
			return;
		}

		/* Kick player because there are no public slots left */
		KickClient(client, "%T", "Slot reserved", client);
	}
}

public OnClientDisconnect_Post(client)
{
	if (GetConVarBool(sm_hide_slots))
	{		
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}
}

SetVisibleMaxSlots(clients, limit)
{
	new num = clients + 1;
	
	if (clients == g_MaxClients)
	{
		num = g_MaxClients;
	} else if (clients < limit) {
		num = limit;
	}
	
	SetConVarInt(sv_visiblemaxplayers, num);
}
