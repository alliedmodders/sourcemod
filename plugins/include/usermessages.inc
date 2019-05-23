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

#if defined _eventsmsgs_included
 #endinput
#endif
#define _eventsmsgs_included

/**
 * UserMsg helper values.
 */
enum UserMsg
{
	INVALID_MESSAGE_ID = -1
};

/**
 * UserMsg message serialization formats
 */
enum UserMessageType
{
	UM_BitBuf = 0,
	UM_Protobuf
};

/**
 * @section Message Flags.
 */
#define USERMSG_RELIABLE        (1<<2)    /**< Message will be set on the reliable stream */
#define USERMSG_INITMSG         (1<<3)    /**< Message will be considered to be an initmsg */
#define USERMSG_BLOCKHOOKS      (1<<7)    /**< Prevents the message from triggering SourceMod and Metamod hooks */

/**
 * @endsection
 */

/**
 * Returns usermessage serialization type used for the current engine
 *
 * @return              The supported usermessage type.
 */
native UserMessageType GetUserMessageType();

stock Protobuf UserMessageToProtobuf(Handle msg)
{
	if (GetUserMessageType() != UM_Protobuf)
	{
		return null;
	}

	return view_as<Protobuf>(msg);
}

// Make sure to only call this on writable buffers (eg from StartMessage).
stock BfWrite UserMessageToBfWrite(Handle msg)
{
	if (GetUserMessageType() == UM_Protobuf)
	{
		return null;
	}

	return view_as<BfWrite>(msg);
}

// Make sure to only call this on readable buffers (eg from a message hook).
stock BfRead UserMessageToBfRead(Handle msg)
{
	if (GetUserMessageType() == UM_Protobuf)
	{
		return null;
	}
	
	return view_as<BfRead>(msg);
}

/**
 * Returns the ID of a given message, or -1 on failure.
 *
 * @param msg           String containing message name (case sensitive).
 * @return              A message index, or INVALID_MESSAGE_ID on failure.
 */
native UserMsg GetUserMessageId(const char[] msg);

/**
 * Retrieves the name of a message by ID.
 *
 * @param msg_id        Message index.
 * @param msg           Buffer to store the name of the message.
 * @param maxlength     Maximum length of string buffer.
 * @return              True if message index is valid, false otherwise.
 */
native bool GetUserMessageName(UserMsg msg_id, char[] msg, int maxlength);

/**
 * Starts a usermessage (network message).
 *
 * @note Only one message can be active at a time.
 * @note It is illegal to send any message while a non-intercept hook is in progress.
 *
 * @param msgname       Message name to start.
 * @param clients       Array containing player indexes to broadcast to.
 * @param numClients    Number of players in the array.
 * @param flags         Optional flags to set.
 * @return              A handle to a bf_write bit packing structure, or
 *                      INVALID_HANDLE on failure.
 * @error               Invalid message name, unable to start a message, invalid client,
 *                      or client not connected.
 */
native Handle StartMessage(const char[] msgname, const int[] clients, int numClients, int flags=0);

/**
 * Starts a usermessage (network message).
 *
 * @note Only one message can be active at a time.
 * @note It is illegal to send any message while a non-intercept hook is in progress.
 *
 * @param msg           Message index to start.
 * @param clients       Array containing player indexes to broadcast to.
 * @param numClients    Number of players in the array.
 * @param flags         Optional flags to set.
 * @return              A handle to a bf_write bit packing structure, or
 *                      INVALID_HANDLE on failure.
 * @error               Invalid message name, unable to start a message, invalid client,
 *                      or client not connected.
 */
native Handle StartMessageEx(UserMsg msg, const int[] clients, int numClients, int flags=0);

/**
 * Ends a previously started user message (network message).
 */
native void EndMessage();

/**
 * Hook function types for user messages.
*/
typeset MsgHook
{
	/**
	 * Called when a bit buffer based usermessage is hooked
	 *
	 * @param msg_id        Message index.
	 * @param msg           Handle to the input bit buffer.
	 * @param players       Array containing player indexes.
	 * @param playersNum    Number of players in the array.
	 * @param reliable      True if message is reliable, false otherwise.
	 * @param init          True if message is an initmsg, false otherwise.
	 * @return              Ignored for normal hooks.  For intercept hooks, Plugin_Handled
	 *                      blocks the message from being sent, and Plugin_Continue
	 *                      resumes normal functionality.
	 */
	function Action (UserMsg msg_id, BfRead msg, const int[] players, int playersNum, bool reliable, bool init);
	/**
	 * Called when a protobuf based usermessage is hooked
	 *
	 * @param msg_id        Message index.
	 * @param msg           Handle to the input protobuf.
	 * @param players       Array containing player indexes.
	 * @param playersNum    Number of players in the array.
	 * @param reliable      True if message is reliable, false otherwise.
	 * @param init          True if message is an initmsg, false otherwise.
	 * @return              Ignored for normal hooks.  For intercept hooks, Plugin_Handled
	 *                      blocks the message from being sent, and Plugin_Continue
	 *                      resumes normal functionality.
	 */
	function Action (UserMsg msg_id, Protobuf msg, const int[] players, int playersNum, bool reliable, bool init);
};

/**
 * Called when a message hook has completed.
 *
 * @param msg_id        Message index.
 * @param sent          True if message was sent, false if blocked.
 */
typedef MsgPostHook = function void (UserMsg msg_id, bool sent);

/**
 * Hooks a user message.
 *
 * @param msg_id        Message index.
 * @param hook          Function to use as a hook.
 * @param intercept     If intercept is true, message will be fully intercepted,
 *                      allowing the user to block the message.  Otherwise,
 *                      the hook is normal and ignores the return value.
 * @param post          Notification function.
 * @error               Invalid message index.
 */
native void HookUserMessage(UserMsg msg_id, MsgHook hook, bool intercept=false, MsgPostHook post=INVALID_FUNCTION);

/**
 * Removes one usermessage hook.
 *
 * @param msg_id        Message index.
 * @param hook          Function used for the hook.
 * @param intercept     Specifies whether the hook was an intercept hook or not.
 * @error               Invalid message index.
 */
native void UnhookUserMessage(UserMsg msg_id, MsgHook hook, bool intercept=false);

/**
 * Starts a usermessage (network message) that broadcasts to all clients.
 *
 * @note See StartMessage or StartMessageEx().
 *
 * @param msgname       Message name to start.
 * @param flags         Optional flags to set.
 * @return              A handle to a bf_write bit packing structure, or
 *                      INVALID_HANDLE on failure.
 */
stock Handle StartMessageAll(const char[] msgname, int flags=0)
{
	int total = 0;
	int[] clients = new int[MaxClients];
	for (int i = 1; i <= MaxClients; i++)
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
 * @note See StartMessage or StartMessageEx().
 *
 * @param msgname       Message name to start.
 * @param client        Client to send to.
 * @param flags         Optional flags to set.
 * @return              A handle to a bf_write bit packing structure, or
 *                      INVALID_HANDLE on failure.
 */
stock Handle StartMessageOne(const char[] msgname, int client, int flags=0)
{
	int players[1];
	players[0] = client;

	return StartMessage(msgname, players, 1, flags);
}
