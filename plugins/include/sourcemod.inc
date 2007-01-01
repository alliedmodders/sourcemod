/**
 * :TODO: license info
 */

#if defined _sourcemod_included
 #endinput
#endif
#define _sourcemod_included

struct Plugin
{
   const String:name[],			/* Plugin Name */
   const String:description[],		/* Plugin Description */
   const String:author[],		/* Plugin Author */
   const String:version[],		/* Plugin Version */
   const String:url[],			/* Plugin URL */
};

#include <float>
#include <handles>
#include <string>
#include <files>

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
forward OnPluginInit(Handle:myself);
 
/**
 *   Called before OnPluginInit, in case the plugin wants to check for load failure.
 * This is called even if the plugin type is "private."  Any natives from modules are 
 * not available at this point.  Thus, this forward should only be used for explicit 
 * pre-emptive things, such as adding dynamic natives, or setting certain types of load filters.
 * 
 * NOTE: It is not safe to call externally resolved natives until OnPluginInit().
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
forward OnPluginUnload();

/**
 * Called when the plugin's pause status is changing.
 *
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
forward OnClientCommand(client);

/**
 * Called whenever the client's settings are changed.
 *
 * @param client		Player index.
 * @noreturn
 */
forward OnClientSettingsChanged(client);

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
 * @return			True on success, false otherwise.
 */
native bool:GetClientName(client, String:name[], maxlen);

/**
 * Retrieves a client's IP address.
 *
 * @param client		Player index.
 * @param name			Buffer to store the client's ip address.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @return			True on success, false otherwise.
 */
native bool:GetClientIP(client, String:ip[], maxlen);

/**
 * Retrieves a client's authentication string (SteamID).
 *
 * @param client		Player index.
 * @param auth			Buffer to store the client's auth string.
 * @param maxlen		Maximum length of string buffer (includes NULL terminator).
 * @return			True on success, false otherwise.
 */
native bool:GetClientAuthString(client, String:auth[], maxlen);

/**
 * Returns if a certain player is connected.
 *
 * @param client		Player index.
 * @return				True if player is connected to the server, false otherwise.
 */
native bool:IsPlayerConnected(client);

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
native bool:IsPlayerAuthorized(client);

/**
 * Returns if a certain player is a fake client.
 *
 * @param client		Player index.
 * @return				True if player is a fake client, false otherwise.
 */
native bool:IsPlayerFakeClient(client);
