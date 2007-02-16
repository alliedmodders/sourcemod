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
#define USERMSG_RELIABLE		(1<<2)		/**< Message will be set to reliable */
#define USERMSG_INITMSG			(1<<3)		/**< Message will be considered to be an initmsg */

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
 * @param msgname		Message name to start.
 * @param clients		Array containing player indexes to broadcast to.
 * @param numClients	Number of players in the array.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
native Handle:StartMessage(String:msgname[], clients[], numClients, flags);

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
native Handle:StartMessageEx(UserMsg:msg, clients[], numClients, flags);

/**
 * Ends a previously started user message (network message).
 *
 * @noreturn
 */
native EndMessage();

/**
 * Starts a usermessage (network message) that broadcasts to all clients.
 *
 * @param msgname		Message name to start.
 * @param flags			Optional flags to set.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
stock Handle:StartMessageAll(String:msgname[], flags=0)
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
	return StartMessage(msgname, clients, total, flags);
}

/**
 * Starts a simpler usermessage (network message) for one client.
 *
 * @param msgname		Message name to start.
 * @param client		Client to send to.
 * @param flags			Optional flags to set.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 */
stock Handle:StartMessageOne(String:msgname[], client, flags=0)
{
	new players[1];
	
	players[0] = client;
	
	return StartMessage(msgname, players, 1, flags);
}
