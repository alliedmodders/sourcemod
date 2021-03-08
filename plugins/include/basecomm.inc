/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2011 AlliedModders LLC.  All rights reserved.
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
 
#if defined _basecomm_included
 #endinput
#endif
#define _basecomm_included

/**
 * Called when a client is muted or unmuted
 * 
 * @param   client      Client index
 * @param   muteState   True if client was muted, false otherwise
 */
forward void BaseComm_OnClientMute(int client, bool muteState);
 
 /**
 * Called when a client is gagged or ungagged
 * 
 * @param   client      Client index
 * @param   gagState    True if client was gaged, false otherwise
 */
forward void BaseComm_OnClientGag(int client, bool gagState);
 
/**
 * Returns whether or not a client is gagged
 *
 * @param client        Client index.
 * @return              True if client is gagged, false otherwise.
 */
native bool BaseComm_IsClientGagged(int client);

/**
 * Returns whether or not a client is muted
 *
 * @param client        Client index.
 * @return              True if client is muted, false otherwise.
 */
native bool BaseComm_IsClientMuted(int client);

/**
 * Sets a client's gag state
 *
 * @param client        Client index.
 * @param gagState      True to gag client, false to ungag.
 * @return              True if this caused a change in gag state, false otherwise.
 */
native bool BaseComm_SetClientGag(int client, bool gagState);

/**
 * Sets a client's mute state
 *
 * @param client        Client index.
 * @param muteState     True to mute client, false to unmute.
 * @return              True if this caused a change in mute state, false otherwise.
 */
native bool BaseComm_SetClientMute(int client, bool muteState);

/* DO NOT EDIT BELOW THIS LINE */

public SharedPlugin __pl_basecomm = 
{
	name = "basecomm",
	file = "basecomm.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_basecomm_SetNTVOptional()
{
	MarkNativeAsOptional("BaseComm_IsClientGagged");
	MarkNativeAsOptional("BaseComm_IsClientMuted");
	MarkNativeAsOptional("BaseComm_SetClientGag");
	MarkNativeAsOptional("BaseComm_SetClientMute");
}
#endif
