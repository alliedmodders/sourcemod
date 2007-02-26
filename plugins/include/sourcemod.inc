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

#if defined _sourcemod_included
 #endinput
#endif
#define _sourcemod_included

struct Plugin
{
   const String:name[],			/* Plugin Name */
   const String:description[],	/* Plugin Description */
   const String:author[],		/* Plugin Author */
   const String:version[],		/* Plugin Version */
   const String:url[],			/* Plugin URL */
};

#include <core>
#include <float>
#include <handles>
#include <string>
#include <admin>
#include <files>
#include <console>
#include <bitbuffer>

/**
 * Declare this as a struct in your plugin to expose its information.
 * Example:
 *
 * public Plugin:myinfo =
 * {
 *    name = "My Plugin",
 *    //etc
 * };
 */
public Plugin:myinfo;
 
/**
 * Called when the plugin is fully initialized and all known external references are resolved.
 * This is called even if the plugin type is "private."
 * NOTE: Errors in this function will cause the plugin to stop running.
 *
 * @noreturn
 */
forward OnPluginStart(Handle:myself);
 
/**
 *   Called before OnPluginStart, in case the plugin wants to check for load failure.
 * This is called even if the plugin type is "private."  Any natives from modules are 
 * not available at this point.  Thus, this forward should only be used for explicit 
 * pre-emptive things, such as adding dynamic natives, or setting certain types of load filters.
 * 
 * NOTE: It is not safe to call externally resolved natives until OnPluginStart().
 * NOTE: Any sort of RTE in this function will cause the plugin to fail loading.
 *
 * @param myself	Handle to the plugin.
 * @param late		Whether or not the plugin was loaded "late" (after map load).
 * @param error		Error message buffer in case load failed.
 * @param err_max	Maximum number of characters for error message buffer.
 * @return			True if load success, false otherwise.
 */
forward bool:AskPluginLoad(Handle:myself, bool:late, String:error[], err_max);

/**
 * Called when the plugin is about to be unloaded.
 *
 * @noreturn
 */
forward OnPluginEnd();

/**
 * Called when the plugin's pause status is changing.
 *
 * @param pause			True if the plugin is being paused, false otherwise.
 * @noreturn
 */
forward OnPluginPauseChange(bool:pause);

/**
 * Called on client connection.
 *
 * @param client		Player index.
 * @param rejectmsg		Buffer to store the rejection message when the connection is refused.
 * @param maxlen		Maximum number of characters for rejection buffer.
 * @return				True to validate client's connection, false to refuse it.
 */
forward bool:OnClientConnect(client, String:rejectmsg[], maxlen);

/**
 * Called when a client is entering to the game.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientPutInServer(client);

/**
 * Called when a client is disconnecting from the server.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientDisconnect(client);

/**
 * Called when a client is disconnected from the server.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientDisconnect_Post(client);

/**
 * Called when a client is sending a command.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientCommand(client, argCount);

/**
 * Called whenever the client's settings are changed.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientSettingsChanged(client);

/**
 * Called when a client receives a Steam ID.
 * @note This is called by bots, but the ID will be "BOT"
 *
 * @param client		Player index.
 * @param auth			Player auth string.
 */
forward OnClientAuthorized(client, const String:auth[]);

/**
 * Called before every server frame.  Note that you should avoid
 * doing expensive computations here, and you should declare large
 * local arrays using 'decl' instead of 'new'.
 */
forward OnGameFrame();

/**
 * Returns the maximum number of clients allowed on the server.
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
native bool:IsPlayerInGame(client);

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
native AddUserFlags(client, {AdminFlag}:...);

/**
 * Removes flags from a client.  If the client is not an admin,
 * this has no effect.
 *
 * @param client		Player's index.
 * @param ...			Flags to remove from the client.
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native RemoveUserFlags(client, {AdminFlag}:...);

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
 * Logs a generic message to the HL2 logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogToGame(const String:format[], {Handle,Float,String,_}:...);

/**
 * Logs a plugin message to the SourceMod logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogMessage(const String:format[], {Handle,Float,String,_}:...);

/**
 * Logs a plugin error message to the SourceMod logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogError(const String:format[], {Handle,Float,String,_}:...);

/**
 * Sets the seed value for the global Half-Life 2 Random Stream
 *
 * @param seed			Seed value.
 * @noreturn	
 */
native SetRandomSeed(seed);

/**
 * Returns a random floating point number from the Half-Life 2 Random Stream
 *
 * @param fMin			Minimum random bound.
 * @param fMax			Maximum random bound.
 * @return				A random number between (inclusive) fMin and fMax.
 */
native Float:GetRandomFloat(Float:fMin=0.0, Float:fMax=1.0);

/**
 * Returns a random number from the Half-Life 2 Random Stream
 *
 * @param nmin			Minimum random bound.
 * @param nmax			Maximum random bound.
 * @return				A random number between (inclusive) nmin and nmax.
 */
native GetRandomInt(nmin, nmax);

/**
 * Returns whether a map is valid or not.
 * 
 * @param 				Map name, excluding .bsp extension.
 * @return				True if valid, false otherwise.
 */
native bool:IsMapValid(const String:map[]);

/**
 * Returns whether the server is dedicated.
 *
 * @return				True if dedicated, false otherwise.
 */
native bool:IsDedicatedServer();

/**
 * Returns the number of entities in the server.
 *
 * @return				Number of entities in the server.
 */
native GetEntityCount();

/**
 * Returns whether or not an entity is valid.
 *
 * @param entity		Index of the entity.
 * @return				True if valid, false otherwise.
 */
native bool:IsValidEntity(entity);

/**
 * Creates a new edict (the basis of a networkable entity)
 *
 * @return				Index of the entity, 0 on failure.
 */
native CreateEdict();

/** 
 * Removes an edict from the world.
 *
 * @param entity		Index of the entity.
 * @noreturn
 * @error				Invalid entity index.
 */
native RemoveEntity(entity);

/**
 * Returns a high-precision time value for profiling the engine.
 *
 * @return				A floating point time value.
 */
native Float:GetEngineTime();

/** 
 * Returns the game time based on the game tick.
 *
 * @return				Game tick time.
 */
native Float:GetGameTime();

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
 * Returns the game description from the mod.
 *
 * @param buffer		Buffer to store the description.
 * @param maxlength		Maximum size of the buffer.
 * @param original		If true, retrieves the original game description,
 *						ignoring any potential hooks from plugins.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 */
native GetGameDescription(String:buffer[], maxlength, bool:original=false);

/**
 * Returns the maximum number of entities.
 *
 * @return				Maximum number of entities.
 */
native GetMaxEntities();

/**
 * Returns the current map name.
 *
 * @param buffer		Buffer to store map name.
 * @param maxlength		Maximum length of buffer.
 * @return				Number of bytes written (UTF-8 safe).
 */
native GetCurrentMap(String:buffer[], maxlength);

#include <usermessages>
#include <helpers>
