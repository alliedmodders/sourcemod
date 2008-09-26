/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _sdktools_tempents_included
 #endinput
#endif
#define _sdktools_tempents_included

/**
 * Called when a temp entity is going to be sent.
 *
 * @param te_name	TE name.
 * @param Players	Array containing target player indexes.
 * @param numClients	Number of players in the array.
 * @param delay		Delay in seconds to send the TE.
 * @return		Plugin_Continue to allow the transmission of the TE, Plugin_Stop to block it.
 */
functag public Action:TEHook(const String:te_name[], const Players[], numClients, Float:delay);

/**
 * Hooks a temp entity.
 *
 * @param te_name	TE name to hook.
 * @param hook		Function to use as a hook.
 * @noreturn
 * @error		Temp Entity name not available or invalid function hook.
 */
native AddTempEntHook(const String:te_name[], TEHook:hook);

/**
 * Removes a temp entity hook.
 *
 * @param te_name	TE name to unhook.
 * @param hook		Function used for the hook.
 * @noreturn
 * @error		Temp Entity name not available or invalid function hook.
 */
native RemoveTempEntHook(const String:te_name[], TEHook:hook);

/**
 * Starts a temp entity transmission.
 *
 * @param te_name	TE name.
 * @noreturn
 * @error		Temp Entity name not available.
 */
native TE_Start(const String:te_name[]);

/**
 * Checks if a certain TE property exists.
 *
 * @param prop		Property to use.
 * @return		True if the property exists, otherwise false.
 */
native bool:TE_IsValidProp(const String:prop[]);

/**
 * Sets an integer value in the current temp entity.
 *
 * @param prop		Property to use.
 * @param value		Integer value to set.
 * @noreturn
 * @error		Property not found.
 */
native TE_WriteNum(const String:prop[], value);

/**
 * Reads an integer value in the current temp entity.
 *
 * @param prop		Property to use.
 * @return		Property value.
 * @error		Property not found.
 */
native TE_ReadNum(const String:prop[]);

/**
 * Sets a floating point number in the current temp entity.
 *
 * @param prop		Property to use.
 * @param value		Floating point number to set.
 * @noreturn
 * @error		Property not found.
 */
native TE_WriteFloat(const String:prop[], Float:value);

/**
 * Reads a floating point number in the current temp entity.
 *
 * @param prop		Property to use.
 * @noreturn		Property value.
 * @error		Property not found.
 */
native Float:TE_ReadFloat(const String:prop[]);

/**
 * Sets a vector in the current temp entity.
 *
 * @param prop		Property to use.
 * @param vector	Vector to set.
 * @noreturn
 * @error		Property not found.
 */
native TE_WriteVector(const String:prop[], const Float:vector[3]);

/**
 * Reads a vector in the current temp entity.
 *
 * @param prop		Property to use.
 * @param vector	Vector to read.
 * @noreturn
 * @error		Property not found.
 */
native TE_ReadVector(const String:prop[], Float:vector[3]);

/**
 * Sets a QAngle in the current temp entity.
 *
 * @param prop		Property to use.
 * @param angles	Angles to set.
 * @return		True on success, otherwise false.
 * @error		Property not found.
 */
native TE_WriteAngles(const String:prop[], const Float:angles[3]);

/**
 * Sets an array of floats in the current temp entity.
 *
 * @param prop		Property to use.
 * @param array		Array of values to copy.
 * @param arraySize	Number of values to copy.
 * @return		True on success, otherwise false.
 * @error		Property not found.
 */
native TE_WriteFloatArray(const String:prop[], const Float:array[], arraySize);

/**
 * Sends the current temp entity to one or more clients.
 *
 * @param clients	Array containing player indexes to broadcast to.
 * @param numClients	Number of players in the array.
 * @param delay		Delay in seconds to send the TE.
 * @noreturn
 * @error		Invalid client index or client not in game.
 */
native TE_Send(clients[], numClients, Float:delay=0.0);

/**
 * Sets an encoded entity index in the current temp entity.
 * (This is usually used for m_nStartEntity and m_nEndEntity).
 *
 * @param prop		Property to use.
 * @param value		Value to set.
 * @noreturn
 * @error		Property not found.
 */
stock TE_WriteEncodedEnt(const String:prop[], value)
{
	new encvalue = (value & 0x0FFF) | ((1 & 0xF)<<12);
	return TE_WriteNum(prop, encvalue);
}

/**
 * Broadcasts the current temp entity to all clients.
 * @note See TE_Start().
 *
 * @param delay		Delay in seconds to send the TE.
 * @noreturn
 */
stock TE_SendToAll(Float:delay=0.0)
{
	new maxClients = GetMaxClients();
	new total = 0;
	new clients[maxClients];
	for (new i=1; i<=maxClients; i++)
	{
		if (IsClientInGame(i))
		{
			clients[total++] = i;
		}
	}
	return TE_Send(clients, total, delay);
}

/**
 * Sends the current TE to only a client.
 * @note See TE_Start().
 *
 * @param client	Client to send to.
 * @param delay		Delay in seconds to send the TE.
 * @noreturn
 * @error		Invalid client index or client not in game.
 */
stock TE_SendToClient(client, Float:delay=0.0)
{
	new players[1];

	players[0] = client;

	return TE_Send(players, 1, delay);
}
