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

/**
 * Plugin public information.
 */
struct Plugin
{
   const String:name[],			/**< Plugin Name */
   const String:description[],	/**< Plugin Description */
   const String:author[],		/**< Plugin Author */
   const String:version[],		/**< Plugin Version */
   const String:url[],			/**< Plugin URL */
};

#include <core>
#include <float>
#include <string>
#include <handles>
#include <functions>
#include <files>
#include <timers>
#include <admin>
#include <dbi>
#include <sorting>
#include <clients>
#include <console>
#include <events>
#include <bitbuffer>
#include <usermessages>
#include <menus>
#include <halflife>

#define MAXPLAYERS		64		/**< Maximum number of players that can be in server */

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
 * @note Errors in this function will cause the plugin to stop running.
 *
 * @noreturn
 */
forward OnPluginStart();
 
/**
 *   Called before OnPluginStart, in case the plugin wants to check for load failure.
 * This is called even if the plugin type is "private."  Any natives from modules are 
 * not available at this point.  Thus, this forward should only be used for explicit 
 * pre-emptive things, such as adding dynamic natives, or setting certain types of load filters.
 * 
 * @note It is not safe to call externally resolved natives until OnPluginStart().
 * @note Any sort of RTE in this function will cause the plugin to fail loading.
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
 * Called before every server frame.  Note that you should avoid
 * doing expensive computations here, and you should declare large
 * local arrays using 'decl' instead of 'new'.
 */
forward OnGameFrame();

/**
 * Called when the map is loaded.
 *
 * @note This used to be OnServerLoad(), which is now deprecated.
 * Plugins still using the old forward will work.
 */
forward OnMapStart();

/**
 * Called right before a map ends.
 */
forward OnMapEnd();

/**
 * Called when the map has loaded and the servercfgfile has finished
 * executing.  This is usually 'server.cfg' but most mods allow it to
 * be set via 'servercfgfile'.  
 *
 * @note If server.cfg is executed more than once per map, this forward
 * will only be called the first time (per map).
 * @note It is not guaranteed that this will be called on all mods. If
 * a mod does not appear to support it, please file a SourceMod bug report.
 */
forward OnServerCfg();

/**
 * Returns the calling plugin's Handle.
 *
 * @return				Handle of the calling plugin.
 */
native Handle:GetMyHandle();

/**
 * Returns an iterator that can be used to search through plugins.
 *
 * @return				Handle to iterate with.  Must be closed via 
 * 						CloseHandle().
 * @error				Invalid Handle.
 */
native Handle:GetPluginIterator();

/**
 * Returns whether there are more plugins available in the iterator.
 *
 * @param iter			Handle to the plugin iterator.
 * @return				True on more plugins, false otherwise.
 * @error				Invalid Handle.
 */
native bool:MorePlugins(Handle:iter);

/**
 * Returns the current plugin in the iterator and advances the iterator.
 *
 * @param iter			Handle to the plugin iterator.
 * @return				Current plugin the iterator is at, before
 *						the iterator is advanced.
 * @error				Invalid Handle.
 */
native Handle:ReadPlugin(Handle:iter);

/**
 * Returns a plugin's status.
 *
 * @param plugin		Plugin Handle (INVALID_HANDLE uses the calling plugin).
 * @return				Status code for the plugin.
 * @error				Invalid Handle.
 */
native PluginStatus:GetPluginStatus(Handle:plugin);

/**
 * Retrieves a plugin's file name relative to the plugins folder.
 *
 * @param plugin		Plugin Handle (INVALID_HANDLE uses the calling plugin).
 * @param buffer		Buffer to the store the file name.
 * @param maxlength		Maximum length of the name buffer.
 * @noreturn
 * @error				Invalid Handle.
 */
native GetPluginFilename(Handle:plugin, String:buffer[], maxlength);

/**
 * Retrieves whether or not a plugin is being debugged.
 *
 * @param plugin		Plugin Handle (INVALID_HANDLE uses the calling plugin).
 * @return				True if being debugged, false otherwise.
 * @error				Invalid Handle.
 */
native bool:IsPluginDebugging(Handle:hndl);

/**
 * Retrieves a plugin's public info.
 *
 * @param plugin		Plugin Handle (INVALID_HANDLE uses the calling plugin).
 * @param info			Plugin info property to retrieve.
 * @param buffer		Buffer to store info in.
 * @param maxlength		Maximum length of buffer.
 * @return				True on success, false if property is not available.
 * @error				Invalid Handle.
 */
native bool:GetPluginInfo(Handle:plugin, PluginInfo:info, String:buffer[], maxlength);

/**
 * Causes the plugin to enter a failed state.  An error will be thrown and
 * the plugin will be paused until it is unloaded or reloaded.
 *
 * @param string		Message to print.
 * @noreturn
 * @error				Always throws SP_ERROR_ABORT.
 */
native SetFailState(const String:string[]);

/**
 * Aborts the current callback and throws an error.  This function
 * does not return in that no code is executed following it.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 * @error				Always!
 */
native ThrowError(const String:fmt[], any:...);

/**
 * Logs a plugin message to the SourceMod logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogMessage(const String:format[], any:...);

/**
 * Logs a plugin error message to the SourceMod logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogError(const String:format[], any:...);

/**
 * Gets the system time as a unix timestamp.
 *
 * @param bigStamp		Optional array to store the 64bit timestamp in.
 * @return				32bit timestamp (number of seconds since unix epoch).
 */
native GetTime(bigStamp[2]={0,0});

/**
 * Loads a game config file.
 *
 * @param file			File to load.  The path must be relative to the 'gamedata' folder under the config folder 
 *					and the extension should be omitted.
 * @return			A handle to the game config file or INVALID_HANDLE in failure.
 */
native Handle:LoadGameConfigFile(const String:file[]);

/**
 * Returns an offset value.
 *
 * @param gc			Game config handle.
 * @param key			Key to retrieve from the offset section.
 * @return			An offset, or -1 on failure.
 */
native GameConfGetOffset(Handle:gc, const String:key[]);

/**
 * Gets the value of a key from the "Keys" section.
 *
 * @param gc			Game config handle.
 * @param key			Key to retrieve from the Keys section.
 * @param buffer		Destination string buffer.
 * @param maxlen		Maximum length of output string buffer.
 * @return				True if key existed, false otherwise.
 */
native bool:GameConfGetKeyValue(Handle:gc, const String:key[], String:buffer[], maxlen);

/**
 * Returns the operating system's "tick count," which is a number of 
 * milliseconds since the operating system loaded.  This can be used
 * for basic benchmarks.
 *
 * @return				Tick count in milliseconds.
 */
native GetSysTickCount();

#include <helpers>
#include <entity>
