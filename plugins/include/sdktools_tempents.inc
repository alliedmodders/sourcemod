/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#if defined _sdktools_tempents_included
 #endinput
#endif
#define _sdktools_tempents_included

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
 * Sets a floating point number in the current temp entity.
 *
 * @param prop		Property to use.
 * @param value		Floating point number to set.
 * @noreturn
 * @error		Property not found.
 */
native TE_WriteFloat(const String:prop[], Float:value);

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
stock TE_SendToOne(client, Float:delay=0.0)
{
	new players[1];

	players[0] = client;

	return TE_Send(players, 1, delay);
}
