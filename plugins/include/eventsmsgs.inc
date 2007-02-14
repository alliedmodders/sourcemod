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
 
#if defined _eventsmsgs_included
 #endinput
#endif
#define _eventsmsgs_included

enum UserMsg
{
	INVALID_MESSAGE_ID = -1,
};

#define USERMSG_PASSTHRU		(1<<0)		/**< Message will pass through other SourceMM plugins */
#define USERMSG_PASSTHRU_ALL	(1<<1)		/**< Message will pass through other SourceMM plugins AND SourceMod */

//:NOTE: none of this is implemented yet, remove this notice and add to sourcemod.inc once it's done!
//:NOTE: for DS, add main event natives below EndMessage but above bf_ stuff

/**
 * Returns the ID of a given message, or -1 on failure.
 *
 * @param msg			String containing message name (case sensitive).
 * @return				A message index, or INVALID_MESSAGE_ID on failure.
 */
native UserMsg:GetUserMessageId(const String:msg[]);

/**
 * Starts a usermessage (network message).
 * @note Only one message can be active at a time.
 *
 * @param msg			Message index to start.  
 * @param clients		Array containing player indexes to broadcast to.
 * @param numClients	Number of players in the array.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
native Handle:StartMessage(UserMsg:msg, clients[], numClients, flags);

/**
 * Ends a previously started user message (network message).
 *
 * @noreturn
 */
native EndMessage();

/** 
 * Writes a single bit to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param bit			Bit to write (true for 1, false for 0).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteBool(Handle:bf, bool:bit);
 
/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param byte			Byte to write (value will be written as 8bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteByte(Handle:bf, byte);

/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param chr			Character to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteChar(Handle:bf, chr);

/**
 * Writes a 16bit integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 16bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteShort(Handle:bf, num);

/**
 * Writes a 16bit unsigned integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 16bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteWord(Handle:bf, num);

/**
 * Writes a normal integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 32bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteNum(Handle:bf, num);

/**
 * Writes a floating point number to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Number to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteFloat(Handle:bf, Float:num);

/**
 * Writes a string to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param string		Text string to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteString(Handle:bf, const String:string[]);

/**
 * Writes an entity to a writable bitbuffer (bf_write).
 * @note This is a wrapper around BfWriteShort().
 *
 * @param bf			bf_write handle to write to.
 * @param ent			Entity index to write.
 * @noreturn
 * @error				Invalid or incorrect Handle, or invalid entity.
 */
native BfWriteEntity(Handle:bf, ent);

/**
 * Writes a bit angle to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param angle			Angle to write.
 * @param numBits		Optional number of bits to use.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteAngle(Handle:bf, Float:angle, numBits=8);

/**
 * Writes a coordinate to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param coord			Coordinate to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteCoord(Handle:bf, Float:coord);

/**
 * Writes a 3D vector of coordinates to a writable bitbuffer (bf_write).
 *
 * @param bif			bf_write handle to write to.
 * @param coord			Coordinate array to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecCoord(Handle:bf, Float:coord[3]);

/**
 * Writes a 3D normal vector to a writable bitbuffer (bf_write).
 *
 * @param bif			bf_write handle to write to.
 * @param vec			Vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecNormal(Handle:bf, Float:vec[3]);

/**
 * Writes a 3D angle vector to a writable bitbuffer (bf_write).
 *
 * @param bif			bf_write handle to write to.
 * @param angles		Angle vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteAngles(Handle:bf, Float:angles[3]);

/**
 * Starts a usermessage (network message) that broadcasts to all clients.
 *
 * @param msg			Message index to start.
 * @param flags			Optional flags to set.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
stock Handle:StartMessageAll(UserMsg:msg, flags=0)
{
	new maxClients = GetMaxClients();
	new total = 0;
	new clients[maxClients];
	for (new i=1; i<=maxClients; i++)
	{
		if (IsClientConnected(i))
		{
			clients[total++] = i;
		}
	}
	return StartMessage(msg, clients, total, flags);
}

/**
 * Starts a simpler usermessage (network message) for one client.
 *
 * @param msg			Message index to start.
 * @param client		Client to send to.
 * @param flags			Optional flags to set.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
stock Handle:StartMessageOne(UserMsg:msg, client, flags=0)
{
	new players[1];
	
	players[0] = client;
	
	return StartMsesage(msg, players, 1, flags);
}
