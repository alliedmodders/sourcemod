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
 * @note It is illegal to send any message while a non-intercept hook is in progress.
 *
 * @param msgname		Message name to start.
 * @param clients		Array containing player indexes to broadcast to.
 * @param numClients	Number of players in the array.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 * @error				Invalid message name or unable to start a message.
 */
native Handle:StartMessage(String:msgname[], clients[], numClients, flags);

/**
 * Starts a usermessage (network message).
 * @note Only one message can be active at a time.
 * @note It is illegal to send any message while a non-intercept hook is in progress.
 *
 * @param msg			Message index to start.
 * @param clients		Array containing player indexes to broadcast to.
 * @param numClients	Number of players in the array.
 * @return				A handle to a bf_write bit packing structure, or
 *						INVALID_HANDLE on failure.
 * @error				Invalid message name or unable to start a message.
 */
native Handle:StartMessageEx(UserMsg:msg, clients[], numClients, flags);

/**
 * Ends a previously started user message (network message).
 *
 * @noreturn
 */
native EndMessage();

/**
 * @brief Called when a message is hooked
 *
 * @param msg_id		Message index.
 * @param players		Array containing player indexes.
 * @param playersNum	Number of players in the array.
 * @param reliable		True if message is reliable, false otherwise.
 * @param init			True if message is an initmsg, false otherwise.
 * @return				Ignored for normal hooks.  For intercept hooks, false blocks 
 *						the message from being sent, and true continues.
 */
functag MsgHook bool:public(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init);

/**
 * @brief Hooks a user message.
 *
 * @param msg_id		Message index.
 * @param hook			Function to use as a hook.
 * @param intercept		If intercept is true, message will be fully intercepted,
 * 						allowing the user to block the message.  Otherwise,
 *						the hook is normal and ignores the return value.
 * @noreturn
 * @error				Invalid message index.
 */
native HookUserMessage(UserMsg:msg_id, MsgHook:hook, bool:intercept=false);

/**
 * @brief Removes one usermessage hook.
 *
 * @param msg_id		Message index.
 * @param hook			Function used for the hook.
 * @param intercept		Specifies whether the hook was an intercept hook or not.
 * @noreturn
 * @error				Invalid message index.
 */
native UnHookUserMessage(UserMsg:msg_id, MsgHook:hook, bool:intercept=false);

/**
 * @brief Called when a message is finished sending.
 *
 * @param msg_id		Message index.
 */
functag MsgSentNotify public(UserMsg:msg_id);

/**
 * @brief Notifies when a message has finished sending.
 *
 * @param msg_id		Message index.
 * @param notify		Notification function.
 * @noreturn
 * @error				Invalid message index.
 */
native NotifyUserMessage(UserMsg:msg_id, MsgSentNotify:notify);

/**
 * @brief Removes a user message notification.
 *
 * @param msg_id		Message index.
 * @param notify		Notification function.
 * @noreturn
 * @error				Invalid message index.
 */
native RemoveNotifyUserMessage(UserMsg:msg_id, MsgSentNotify:notify);

/**
 * Starts a usermessage (network message) that broadcasts to all clients.
 * @note See StartMessage or StartMessageEx().
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
 * @note See StartMessage or StartMessageEx().
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
