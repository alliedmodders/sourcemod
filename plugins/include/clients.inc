/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

#if defined _clients_included
 #endinput
#endif
#define _clients_included

enum NetFlow
{
	NetFlow_Outgoing = 0,	/**< Outgoing traffic */
	NetFlow_Incoming,		/**< Incoming traffic */
	NetFlow_Both			/**< Incoming and outgoing traffic */
};

#define MAXPLAYERS		64	/**< Maximum number of players that can be in server */
#define MAX_NAME_LENGTH 32	/**< Maximum buffer required to store a client name */

/**
 * Called on client connection.
 *
 * @param client		Client index.
 * @param rejectmsg		Buffer to store the rejection message when the connection is refused.
 * @param maxlen		Maximum number of characters for rejection buffer.
 * @return				True to validate client's connection, false to refuse it.
 */
forward bool:OnClientConnect(client, String:rejectmsg[], maxlen);

/**
 * Called when a client is entering to the game.
 *
 * @note Whether a client has a steamid is undefined until OnClientAuthorized
 * is called, which may occur either before or after OnClientPutInServer.
 *
 * @param client		Client index.
 * @noreturn
 */
forward OnClientPutInServer(client);

/**
 * Called when a client is disconnecting from the server.
 *
 * @param client		Client index.
 * @noreturn
 */
forward OnClientDisconnect(client);

/**
 * Called when a client is disconnected from the server.
 *
 * @param client		Client index.
 * @noreturn
 */
forward OnClientDisconnect_Post(client);

/**
 * Called when a client is sending a command.
 *
 * @param client		Client index.
 * @param args			Number of arguments.
 * @noreturn
 */
forward Action:OnClientCommand(client, args);

/**
 * Called whenever the client's settings are changed.
 *
 * @param client		Client index.
 * @noreturn
 */
forward OnClientSettingsChanged(client);

/**
 * Called when a client receives a Steam ID.
 
 * Admin properties are not assigned here, but on OnClientPostAdminCheck().
 * This is called by bots, but the ID will be "BOT".
 *
 * @param client		Client index.
 * @param auth			Client auth string.
 * @noreturn
 */
forward OnClientAuthorized(client, const String:auth[]);

/**
 * Called once a client is authorized and fully in-game, but 
 * before admin checks are done.  This can be used to override 
 * the default admin checks for a client.
 *
 * Note: If handled/blocked, PostAdminCheck must be signalled 
 * manually via NotifyPostAdminCheck().
 *
 * @param client		Client index.
 * @return				Plugin_Handled to block admin checks.
 */
forward Action:OnClientPreAdminCheck(client);

/**
 * Called once a client is authorized and fully in-game, and 
 * after all post-connection authorizations have been performed.
 *
 * @param client		Client index.
 * @noreturn
 */
forward OnClientPostAdminCheck(client);

/**
 * Returns the maximum number of clients allowed on the server.  This may 
 * return 0 if called before OnMapStart(), and thus should not be called 
 * in OnPluginStart().  
 *
 * @return				Maximum number of clients allowed.
 */
native GetMaxClients();

/**
 * Returns the client count put in the server.
 *
 * @param inGameOnly	If false connecting players are also counted.
 * @return				Client count in the server.
 */
native GetClientCount(bool:inGameOnly=true);

/**
 * Returns the client's name.
 *
 * @param client		Player index.
 * @param name			Buffer to store the client's name.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @return				True on success, false otherwise.
 * @error				If the client is not connected an error will be thrown.
 */
native bool:GetClientName(client, String:name[], maxlen);

/**
 * Retrieves a client's IP address.
 *
 * @param client		Player index.
 * @param name			Buffer to store the client's ip address.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @param remport		Remove client's port from the ip string (true by default).
 * @return				True on success, false otherwise.
 * @error				If the client is not connected or the index is invalid.
 */
native bool:GetClientIP(client, String:ip[], maxlen, bool:remport=true);

/**
 * Retrieves a client's authentication string (SteamID).
 *
 * @param client		Player index.
 * @param auth			Buffer to store the client's auth string.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @return				True on success, false otherwise.
 * @error				If the client is not connected or the index is invalid.
 */
native bool:GetClientAuthString(client, String:auth[], maxlen);

/**
 * Retrieves a client's user id, which is an index incremented for every client
 * that joins the server.
 *
 * @param client		Player index.
 * @return				User id of the client.
 * @error 				If the client is not connected or the index is invalid.
 */
native GetClientUserId(client);

/**
 * Returns if a certain player is connected.
 *
 * @param client		Player index.
 * @return				True if player is connected to the server, false otherwise.
 */
native bool:IsClientConnected(client);

/**
 * Returns if a certain player has entered the game.
 *
 * @param client		Player index.
 * @return				True if player has entered the game, false otherwise.
 */
native bool:IsClientInGame(client);

/**
 * Backwards compatibility stock - use IsClientInGame
 * @deprecated			Renamed to IsClientInGame
 */
#pragma deprecated Use IsClientInGame() instead
stock bool:IsPlayerInGame(client)
{
	return IsClientInGame(client);
}

/**
 * Returns if a certain player has been authenticated.
 *
 * @param client		Player index.
 * @return				True if player has been authenticated, false otherwise.
 */
native bool:IsClientAuthorized(client);

/**
 * Returns if a certain player is a fake client.
 *
 * @param client		Player index.
 * @return				True if player is a fake client, false otherwise.
 */
native bool:IsFakeClient(client);

/**
 * Returns if a certain player is an observer/spectator.
 *
 * @param client		Player index.
 * @return				True if player is an obverser, false otherwise.
 */
native bool:IsClientObserver(client);

/**
 * Retrieves values from client replicated keys.
 *
 * @param client		Player's index.
 * @param key			Key string.
 * @param value			Buffer to store value.
 * @param maxlen		Maximum length of valve (UTF-8 safe).
 * @return 				True on success, false otherwise.
 * @error				Invalid client index, or client not connected.
 */
native bool:GetClientInfo(client, const String:key[], String:value[], maxlen);

/**
 * Retrieves a client's team index.
 *
 * @param client		Player's index.
 * @return				Team index the client is on (mod specific).
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientTeam(client);
 
/**
 * Sets a client's AdminId.  
 *
 * @param client		Player's index.
 * @param id			AdminId to set.  INVALID_ADMIN_ID removes admin permissions.
 * @param temp			True if the id should be freed on disconnect.
 * @noreturn
 * @error				Invalid client index, client not connected, or bogus AdminId.
 */
native SetUserAdmin(client, AdminId:id, bool:temp=false);

/**
 * Retrieves a client's AdminId.
 *
 * @param client		Player's index.
 * @return				AdminId of the client, or INVALID_ADMIN_ID if none.
 * @error				Invalid client index, or client not connected.
 */
native AdminId:GetUserAdmin(client);

/**
 * Sets access flags on a client.  If the client is not an admin,
 * a temporary, anonymous AdminId is given.
 *
 * @param client		Player's index.
 * @param ...			Flags to set on the client.
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native AddUserFlags(client, AdminFlag:...);

/**
 * Removes flags from a client.  If the client is not an admin,
 * this has no effect.
 *
 * @param client		Player's index.
 * @param ...			Flags to remove from the client.
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native RemoveUserFlags(client, AdminFlag:...);

/** 
 * Sets access flags on a client using bits instead of flags.  If the
 * client is not an admin, and flags not 0, a temporary, anonymous AdminId is given.
 *
 * @param client		Player's index.
 * @param flags			Bitstring of flags to set on client.
 * @noreturn
 */
native SetUserFlagBits(client, flags);

/**
 * Returns client access flags.  If the client is not an admin,
 * the result is always 0.
 * 
 * @param client		Player's index.
 * @return				Flags
 * @error				Invalid client index, or client not connected.
 */
native GetUserFlagBits(client);

/**
 * Returns whether a user can target another user.
 * This is a helper function for CanAdminTarget.
 *
 * @param client		Player's index.
 * @param target		Target player's index.
 * @return				True if target is targettable by the player, false otherwise.
 * @error				Invalid or unconnected player indexers.
 */
native bool:CanUserTarget(client, target);

/**
 * Runs through the Core-defined admin authorization checks on a player.
 * Has no effect if the player is already an admin.
 *
 * Note: This function is based on the internal cache only.
 *
 * @param client		Client index.
 * @return				True if access was changed, false if it did not.
 * @error				Invalid client index or client not in-game AND authorized.
 */
native bool:RunAdminCacheChecks(client);

/**
 * Signals that a player has completed post-connection admin checks.
 * Has no effect if the player has already had this event signalled.
 *
 * Note: This must be sent even if no admin id was assigned.
 *
 * @param client		Client index.
 * @noreturn
 * @error				Invalid client index or client not in-game AND authorized.
 */
native NotifyPostAdminCheck(client);

/** 
 * Creates a fake client.
 *
 * @param name			Name to use.
 * @return				Client index on success, 0 otherwise.
 */
native CreateFakeClient(const String:name[]);

/**
 * Sets a convar value on a fake client.
 *
 * @param client		Client index.
 * @param cvar			ConVar name.
 * @param value			ConVar value.
 * @noreturn
 * @error				Invalid client index, client not connected,
 *						or client not a fake client.
 */
native SetFakeClientConVar(client, const String:cvar[], const String:value[]);

/**
 * Returns the client's health.
 *
 * @param client		Player's index.
 * @return				Health value.
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientHealth(client);

/**
 * Returns the client's model name.
 *
 * @param client		Player's index.
 * @param model			Buffer to store the client's model name.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientModel(client, String:model[], maxlen);

/**
 * Returns the client's weapon name.
 *
 * @param client		Player's index.
 * @param weapon		Buffer to store the client's weapon name.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientWeapon(client, String:weapon[], maxlen);

/**
 * Returns the client's max size vector.
 *
 * @param client		Player's index.
 * @param vec			Destination vector to store the client's max size.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientMaxs(client, Float:vec[3]);

/**
 * Returns the client's min size vector.
 *
 * @param client		Player's index.
 * @param vec			Destination vector to store the client's min size.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientMins(client, Float:vec[3]);

/**
 * Returns the client's position angle.
 *
 * @param client		Player's index.
 * @param ang			Destination vector to store the client's position angle.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientAbsAngles(client, Float:ang[3]);

/**
 * Returns the client's origin vector.
 *
 * @param client		Player's index.
 * @param vec			Destination vector to store the client's origin vector.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientAbsOrigin(client, Float:vec[3]);

/**
 * Returns the client's armor.
 *
 * @param client		Player's index.
 * @return				Armor value.
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientArmor(client);

/**
 * Returns the client's death count.
 *
 * @param client		Player's index.
 * @return				Death count.
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientDeaths(client);

/**
 * Returns the client's frag count.
 *
 * @param client		Player's index.
 * @return				Frag count.
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientFrags(client);

/**
 * Returns the client's send data rate in bytes/sec.
 *
 * @param client		Player's index.
 * @return				Data rate.
 * @error				Invalid client index, client not in game, or fake client.
 */
native GetClientDataRate(client);

/**
 * Returns if a client is timing out
 *
 * @param client		Player's index.
 * @return				True if client is timing out, false otherwise.
 * @error				Invalid client index, client not in game, or fake client.
 */
native bool:IsClientTimingOut(client);

/**
 * Returns the client's connection time in seconds.
 *
 * @param client		Player's index.
 * @return				Connection time.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientTime(client);

/**
 * Returns the client's current latency (RTT), more accurate than GetAvgLatency but jittering.
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Latency.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientLatency(client, NetFlow:flow);

/**
 * Returns the client's average packet latency in seconds.
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Average latency.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientAvgLatency(client, NetFlow:flow);

/**
 * Returns the client's average packet loss, values go from 0 to 1 (for percentages).
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Average packet loss.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientAvgLoss(client, NetFlow:flow);

/**
 * Returns the client's average packet choke, values go from 0 to 1 (for percentages).
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Average packet choke.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientAvgChoke(client, NetFlow:flow);

/**
 * Returns the client's data flow in bytes/sec.
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Data flow.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientAvgData(client, NetFlow:flow);

/**
 * Returns the client's average packet frequency in packets/sec.
 *
 * @param client		Player's index.
 * @param flow			Traffic flowing direction.
 * @return				Packet frequency.
 * @error				Invalid client index, client not in game, or fake client.
 */
native Float:GetClientAvgPackets(client, NetFlow:flow);

/**
 * Translates an userid index to the real player index.
 *
 * @param userid		Userid value.
 * @return				Client value.
 * @error				Returns 0 if invalid userid.
 */
native GetClientOfUserId(userid);

/**
 * Disconnects a client from the server.  
 *
 * Kicking clients from certain events or callbacks may cause crashes.  If in 
 * doubt, create a short (0.1 second) timer to kick the client in the next 
 * available frame.
 *
 * @param client		Client index.
 * @param format		Optional formatting rules for disconnect reason.
 *                      Note that a period is automatically appended to the string by the engine.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native KickClient(client, const String:format[]="", any:...);

/**
 * Changes a client's team through the mod's generic team changing function.
 * On CS:S, this will kill the player.
 *
 * @param client		Client index.
 * @param team			Mod-specific team index.
 * @noreturn
 * @error				Invalid client index, client not connected, or lack of 
 *						mod support.
 */
native ChangeClientTeam(client, team);
