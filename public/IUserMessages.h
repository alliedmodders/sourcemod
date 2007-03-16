/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
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

	#define USERMSG_PASSTHRU			(1<<0)		/**< Message will pass through other SourceMM plugins */
	#define USERMSG_PASSTHRU_ALL		(1<<1)		/**< Message will pass through other SourceMM plugins AND SourceMod */
	#define USERMSG_RELIABLE			(1<<2)		/**< Message will be set to reliable */
	#define USERMSG_INITMSG				(1<<3)		/**< Message will be considered to be an initmsg */

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
		 * Note: due to a bug in Valve's earlier SDK versions, this
		 * may cause crashes alongside IServerGameDLL003 if the message is
		 * not found.
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
