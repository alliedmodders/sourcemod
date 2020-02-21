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

#if defined _sdktools_voice_included
 #endinput
#endif
#define _sdktools_voice_included

/**
 * @section voice flags.
 */
#define VOICE_NORMAL        0   /**< Allow the client to listen and speak normally. */
#define VOICE_MUTED         1   /**< Mutes the client from speaking to everyone. */
#define VOICE_SPEAKALL      2   /**< Allow the client to speak to everyone. */
#define VOICE_LISTENALL     4   /**< Allow the client to listen to everyone. */
#define VOICE_TEAM          8   /**< Allow the client to always speak to team, even when dead. */
#define VOICE_LISTENTEAM    16  /**< Allow the client to always hear teammates, including dead ones. */

/**
 * @endsection
 */

enum ListenOverride
{
	Listen_Default = 0, /**< Leave it up to the game */
	Listen_No,          /**< Can't hear */
	Listen_Yes          /**< Can hear */
};

/**
 * Set the client listening flags.
 *
 * @param client        The client index
 * @param flags         The voice flags
 */
native void SetClientListeningFlags(int client, int flags);

/**
 * Retrieve the client current listening flags.
 *
 * @param client        The client index
 * @return              The current voice flags
 */
native int GetClientListeningFlags(int client);

/**
 * Set the receiver ability to listen to the sender.
 *
 * @param iReceiver     The listener index.
 * @param iSender       The sender index.
 * @param bListen       True if the receiver can listen to the sender, false otherwise.
 * @return              True if successful otherwise false.
 * @deprecated          Use SetListenOverride() instead.
 */
#pragma deprecated Use SetListenOverride() instead
native bool SetClientListening(int iReceiver, int iSender, bool bListen);

/**
 * Retrieves if the receiver can listen to the sender.
 *
 * @param iReceiver     The listener index.
 * @param iSender       The sender index.
 * @return              True if successful otherwise false.
 * @deprecated          GetListenOverride() instead.
 */
#pragma deprecated GetListenOverride() instead
native bool GetClientListening(int iReceiver, int iSender);

/**
 * Override the receiver's ability to listen to the sender.
 *
 * @param iReceiver     The listener index.
 * @param iSender       The sender index.
 * @param override      The override of the receiver's ability to listen to the sender.
 * @return              True if successful otherwise false.
 */
native bool SetListenOverride(int iReceiver, int iSender, ListenOverride override);

/**
 * Retrieves the override of the receiver's ability to listen to the sender.
 *
 * @param iReceiver     The listener index.
 * @param iSender       The sender index.
 * @return              The override value.
 */
native ListenOverride GetListenOverride(int iReceiver, int iSender);

/**
 * Retrieves if the muter has muted the mutee.
 *
 * @param iMuter        The muter index.
 * @param iMutee        The mutee index.
 * @return              True if muter has muted mutee, false otherwise.
 */
native bool IsClientMuted(int iMuter, int iMutee);
