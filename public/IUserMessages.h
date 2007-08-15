/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_USERMESSAGES_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_USERMESSAGES_H_

#include <IShareSys.h>
#include <sp_vm_api.h>
#include <IForwardSys.h>
#include <bitbuf.h>
#include <irecipientfilter.h>

/**
 * @file IUserMessages.h
 * @brief Contains functions for advanced usermessage hooking.
 */

#define SMINTERFACE_USERMSGS_NAME		"IUserMessages"
#define SMINTERFACE_USERMSGS_VERSION	1

namespace SourceMod
{
	/**
	 * @brief Listens to user messages sent from the server.
	 */
	class IUserMessageListener
	{
	public:
		/**
		 * @brief Called when a hooked user message is being sent 
		 * and all interceptions have finished.
		 *
		 * @param msg_id		Message Id.
		 * @param bf			bf_write structure containing written bytes.
		 * @param pFilter		Recipient filter.
		 */
		virtual void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
		{
		}

		/**
		 * @brief Called when a hooked user message is intercepted.
		 * 
		 * @param msg_id		Message Id.
		 * @param bf			bf_write structure containing written bytes.
		 * @param pFilter		Recipient filter.
		 * @return				Pl_Continue to allow message, Pl_Stop or Pl_Handled to scrap it.
		 */
		virtual ResultType InterceptUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
		{
			return Pl_Continue;
		}

		/**
		 * @brief Called when a hooked user message is sent, regardless of the hook type.
		 * @param msg_id		Message Id.
		 */
		virtual void OnUserMessageSent(int msg_id)
		{
		}
	};

	#define USERMSG_RELIABLE			(1<<2)		/**< Message will be set to reliable */
	#define USERMSG_INITMSG				(1<<3)		/**< Message will be considered to be an initmsg */
	#define USERMSG_BLOCKHOOKS			(1<<7)		/**< Prevents the message from triggering SourceMod and Metamod hooks */

	/**
	 * @brief Contains functions for hooking user messages.
	 */
	class IUserMessages : public SMInterface
	{
	public:
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_USERMSGS_VERSION;
		}
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_USERMSGS_NAME;
		}
	public:
		/**
		 * @brief Finds a message id by name.
		 *
		 * @param msg		Case-sensitive string containing the message.
		 * @return			A message index, or -1 on failure.
		 */
		virtual int GetMessageIndex(const char *msg) =0;

		/**
		 * @brief Sets a hook on a user message.
		 *
		 * @param msg_id		Message Id.
		 * @param pListener		Pointer to an IUserMessageListener.
		 * @param intercept		If true, message will be intercepted rather than merely hooked.
		 * @return				True on success, false otherwise.
		 */
		virtual bool HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false) =0;

		/**
		 * @brief Unhooks a user message.
		 *
		 * @param msg_id		Message Id.
		 * @param pListener		Pointer to an IUserMessageListener.
		 * @param intercept		If true, removed message will from interception pool rather than normal hook pool.
		 * @return				True on success, false otherwise.
		 */
		virtual bool UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false) =0;

		/**
		 * @brief Wrapper around UserMessageBegin for more options.
		 * 
		 * @param msg_id		Message Id.
		 * @param players		Array containing player indexes.
		 * @param playersNum	Number of players in the array.
		 * @param flags			Flags to use for sending the message.
		 * @return				bf_write structure to write message with, or NULL on failure.
		 */
		virtual bf_write *StartMessage(int msg_id, cell_t players[], unsigned int playersNum, int flags) =0;

		/**
		 * @brief Wrapper around UserMessageEnd for use with StartMessage().
		 * @return				True on success, false otherwise.
		 */
		virtual bool EndMessage() =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_USERMESSAGES_H_
