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

#if defined _sdktools_voice_included
  #endinput
#endif
#define _sdktools_voice_included

/**
 * @section voice flags.
 */
#define VOICE_NORMAL		0	/**< Allow the client to listen and speak normally. */
#define VOICE_MUTED			1	/**< Mutes the client from speaking to everyone. */
#define VOICE_SPEAKALL		2	/**< Allow the client to speak to everyone. */
#define VOICE_LISTENALL		4	/**< Allow the client to listen to everyone. */
 
/**
 * @endsection
 */

/**
 * Set the client listening flags.
 *
 * @param client		The client index
 * @param flags		The voice flags
 * @noreturn
 */
native SetClientListeningFlags(client, flags);

/**
 * Retrieve the client current listening flags.
 *
 * @param client		The client index
 * @return			The current voice flags
 */
native GetClientListeningFlags(client);

/**
 * Set the receiver ability to listen to the sender.
 *
 * @param iReceiver		The listener index.
 * @param iSender		The sender index.
 * @return			True if successful otherwise false.
 */
native bool:SetClientListening(iReceiver, iSender, bool:bListen);

/**
 * Retrieves if the receiver can listen to the sender.
 *
 * @param iReceiver		The listener index.
 * @param iSender		The sender index.
 * @return			True if successful otherwise false.
 */
native bool:GetClientListening(iReceiver, iSender);