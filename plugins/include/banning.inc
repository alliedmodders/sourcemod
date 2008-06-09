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

#if defined _banning_included
 #endinput
#endif
#define _banning_included

#define BANFLAG_AUTO		(1<<0)	/**< Auto-detects whether to ban by steamid or IP */
#define BANFLAG_IP			(1<<1)	/**< Always ban by IP address */
#define BANFLAG_AUTHID		(1<<2)	/**< Always ban by authstring (for BanIdentity) if possible */
#define BANFLAG_NOKICK		(1<<3)	/**< Does not kick the client */

/**
 * Called for calls to BanClient() with a non-empty command.
 *
 * @param client		Client being banned.
 * @param time			Time the client is being banned for (0 = permanent).
 * @param flags			One if AUTHID or IP will be enabled.  If AUTO is also 
 *						enabled, it means Core autodetected which to use.
 * @param reason		Reason passed via BanClient().
 * @param kick_message	Kick message passed via BanClient().
 * @param command		Command string to identify the ban source.
 * @param source		Source value passed via BanClient().
 * @return				Plugin_Handled to block the actual server banning.
 *						Kicking will still occur.
 */
forward Action:OnBanClient(client,
						   time, 
						   flags, 
						   const String:reason[], 
						   const String:kick_message[], 
						   const String:command[],
						   any:source);

/**
 * Called for calls to BanIdentity() with a non-empty command.
 *
 * @param identity		Identity string being banned (authstring or ip).
 * @param time			Time the client is being banned for (0 = permanent).
 * @param flags			Ban flags (only IP or AUTHID are valid here).
 * @param reason		Reason passed via BanIdentity().
 * @param command		Command string to identify the ban source.
 * @param source		Source value passed via BanIdentity().
 * @return				Plugin_Handled to block the actual server banning.
 */
forward Action:OnBanIdentity(const String:identity[],
							 time,
							 flags,
							 const String:reason[],
							 const String:command[],
							 any:source);

/**
 * Called for calls to RemoveBan() with a non-empty command.
 *
 * @param identity		Identity string being banned (authstring or ip).
 * @param flags			Ban flags (only IP or AUTHID are valid here).
 * @param command		Command string to identify the ban source.
 * @param source		Source value passed via BanIdentity().
 * @return				Plugin_Handled to block the actual server banning.
 */
forward Action:OnRemoveBan(const String:identity[],
						   flags,
						   const String:command[],
						   any:source);

/**
 * Bans a client.
 *
 * @param client		Client being banned.
 * @param time			Time (in minutes) to ban (0 = permanent).
 * @param flags			Flags for controlling the ban mechanism.  If AUTHID 
 *						is set and no AUTHID is available, the ban will fail 
 *						unless AUTO is also flagged.
 * @param reason		Reason to ban the client for.
 * @param kick_message	Message to display to the user when kicking.
 * @param command		Command string to identify the source.  If this is left 
 *						empty, then the OnBanClient forward will not be called.
 * @param source		A source value that could be interpreted as a player 
 *						index of any sort (not actually checked by Core).
 * @return				True on success, false on failure.
 * @error				Invalid client index or client not in game.
 */
native bool:BanClient(client, 
					  time, 
					  flags, 
					  const String:reason[], 
					  const String:kick_message[]="", 
					  const String:command[]="",
					  any:source=0);

/**
 * Bans an identity (either an IP address or auth string).
 *
 * @param identity		String to ban (ip or authstring).
 * @param time			Time to ban for (0 = permanent).
 * @param flags			Flags (only IP and AUTHID are valid flags here).
 * @param reason		Ban reason string.
 * @param command		Command string to identify the source.  If this is left 
 *						empty, then the OnBanIdentity forward will not be called.
 * @param source		A source value that could be interpreted as a player
 *						index of any sort (not actually checked by Core).
 * @return				True on success, false on failure.
 */
native bool:BanIdentity(const String:identity[], 
						time, 
						flags, 
						const String:reason[],
						const String:command[]="",
						any:source=0);

/**
 * Removes a ban that was written to the server (either in memory or on disk).
 *
 * @param identity		String to unban (ip or authstring).
 * @param flags			Flags (only IP and AUTHID are valid flags here).
 * @param command		Command string to identify the source.  If this is left 
 *						empty, then OnRemoveBan will not be called.
 * @param source		A source value that could be interpreted as a player 
 *						index of any sort (not actually checked by Core).
 * @return				True on success, false on failure.
 */
native bool:RemoveBan(const String:identity[], 
					  flags, 
					  const String:command[]="", 
					  any:source=0);

