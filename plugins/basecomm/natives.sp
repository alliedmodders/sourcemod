/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecomm
 * Part of Basecomm plugin, menu and other functionality.
 *
 * SourceMod (C)2004-2011 AlliedModders LLC.  All rights reserved.
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
 
 public int Native_IsClientGagged(Handle hPlugin, int numParams)
{
	int client = GetNativeCell(1);
	if (client < 1 || client > MaxClients)
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Invalid client index %d", client);
	}
	
	if (!IsClientInGame(client))
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Client %d is not in game", client);
	}
	
	return playerstate[client].isGagged;
}

public int Native_IsClientMuted(Handle hPlugin, int numParams)
{
	int client = GetNativeCell(1);
	if (client < 1 || client > MaxClients)
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Invalid client index %d", client);
	}
	
	if (!IsClientInGame(client))
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Client %d is not in game", client);
	}
	
	return playerstate[client].isMuted;
}

public int Native_SetClientGag(Handle hPlugin, int numParams)
{
	int client = GetNativeCell(1);
	if (client < 1 || client > MaxClients)
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Invalid client index %d", client);
	}
	
	if (!IsClientInGame(client))
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Client %d is not in game", client);
	}
	
	bool gagState = GetNativeCell(2);
	
	if (gagState)
	{
		if (playerstate[client].isGagged)
		{
			return false;
		}
		
		PerformGag(-1, client, true);
	}
	else
	{
		if (!playerstate[client].isGagged)
		{
			return false;
		}
		
		PerformUnGag(-1, client, true);
	}
	
	return true;
}

public int Native_SetClientMute(Handle hPlugin, int numParams)
{
	int client = GetNativeCell(1);
	if (client < 1 || client > MaxClients)
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Invalid client index %d", client);
	}
	
	if (!IsClientInGame(client))
	{
		return ThrowNativeError(SP_ERROR_NATIVE, "Client %d is not in game", client);
	}
	
	bool muteState = GetNativeCell(2);
	
	if (muteState)
	{
		if (playerstate[client].isMuted)
		{
			return false;
		}
		
		PerformMute(-1, client, true);
	}
	else
	{
		if (!playerstate[client].isMuted)
		{
			return false;
		}
		
		PerformUnMute(-1, client, true);
	}
	
	return true;
}
