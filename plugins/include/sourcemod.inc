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
#include <vector>
#include <string>
#include <handles>
#include <functions>
#include <files>
#include <timers>
#include <admin>
#include <dbi>
#include <lang>
#include <sorting>
#include <textparse>
#include <clients>
#include <console>
#include <events>
#include <bitbuffer>
#include <usermessages>
#include <keyvalues>
#include <menus>
#include <halflife>
#include <adt>
#include <banning>

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
 * Called before OnPluginStart, in case the plugin wants to check for load failure.
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
 * Called when the map has loaded, servercfgfile (server.cfg) has been 
 * executed, and all plugin configs are done executing.  This is the best
 * place to inialize plugin functions which are based on cvar data.  
 *
 * @note This will always be called once and only once per map.  It will be 
 * called after OnMapStart().
 *
 * @noreturn
 */
forward OnConfigsExecuted();

/**
 * @deprecated Use OnConfigsExecuted() instead.
 */
#pragma deprecated Use OnConfigsExecuted() instead
forward OnServerCfg();

/**
 * Called after all plugins have been loaded.
 */
forward OnAllPluginsLoaded();

/**
 * Called when an action is going to be logged.
 *
 * @param source		Handle to the object logging the action, or INVALID_HANDLE 
 *						if Core is logging the action.
 * @param ident			Type of object logging the action (plugin, ext, or core).
 * @param client		Client the action is from; 0 for server, -1 if not applicable.
 * @param target		Client the action is targetting, or -1 if not applicable.
 * @param message		Message that is being logged.
 * @return				Plugin_Continue will cause Core to defaulty log the message.
 *						Plugin_Handled will stop Core from logging the message.
 * 						Plugin_Stop is the same as Handled, but prevents any other 
 *						plugins from handling the message.
 */
forward Action:OnLogAction(Handle:source, 
						   Identity:ident,
						   client,
						   target,
						   const String:message[]);

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
 * Logs a plugin message to the SourceMod logs.  The log message will be 
 * prefixed by the plugin's logtag (filename).
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogMessage(const String:format[], any:...);

/**
 * Logs a message to the SourceMod logs without any plugin logtag.  This is 
 * useful for re-routing messages from other plugins, for example, messages 
 * from LogAction().
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogMessageEx(const String:format[], any:...);

/**
 * Logs a message to any file.  The log message will be in the normal 
 * SourceMod format, with the plugin logtag prepended.
 *
 * @param file			File to write the log message in.
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 * @error				File could not be opened/written.
 */
native LogToFile(const String:file[], const String:format[], any:...);

/**
 * Same as LogToFile(), except no plugin logtag is prepended.
 *
 * @param file			File to write the log message in.
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 * @error				File could not be opened/written.
 */
native LogToFileEx(const String:file[], const String:format[], any:...);

/**
 * Logs an action from a command or event whereby interception and routing may 
 * be important.  This is intended to be a logging version of ShowActivity().
 *
 * @param client		Client performing the action, 0 for server, or -1 if not 
 *						applicable.
 * @param target		Client being targetted, or -1 if not applicable.
 * @param message		Message format.
 * @param ...			Message formatting parameters.
 * @noreturn
 */
native LogAction(client, target, const String:message[], any:...);

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
 * Produces a date and/or time string value for a timestamp.
 *
 * See this URL for valid parameters: 
 * http://opengroup.org/onlinepubs/007908799/xsh/strftime.html
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules (passing NULL_STRING will use the rules defined in sm_datetime_format).
 * @param stamp			Optional time stamp.
 * @noreturn
 * @error			Buffer too small or invalid time format.
 */
native FormatTime(String:buffer[], maxlength, const String:format[], stamp=-1);

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

/**
 * Specifies that the given config file should be executed after plugin load.
 * OnConfigsExecuted() will not be called until the config file has executed, 
 * but it will be called if the execution fails.
 *
 * @param autoCreate	If true, and the config file does not exist, such a config
 *						file will be automatically created and populated with
 *						information from the plugin's registered cvars.
 * @param name			Name of the config file, excluding the .cfg extension.
 *						If empty, <plugin.filename.cfg> is assumed.
 * @param folder		Folder under cfg/ to use.  By default this is "sourcemod."
 * @noreturn
 */
native AutoExecConfig(bool:autoCreate=true, const String:name[]="", const String:folder[]="sourcemod");

/**
 * Sets a native as optional, such that if it is unloaded, removed,
 * or otherwise non-existent, the plugin will still work.  Calling
 * removed natives results in a run-time error.
 *
 * @param name			Native name.
 * @noreturn
 */
native MarkNativeAsOptional(const String:name[]);

/**
 * Registers a library name for identifying as a dependency to 
 * other plugins.
 *
 * @param name			Library name.
 * @noreturn
 */
native RegPluginLibrary(const String:name[]);

/**
 * Returns whether a library exists.  This function should be considered 
 * expensive; it should only be called on plugin to determine availability 
 * of resources.  Use OnLibraryAdded()/OnLibraryRemoved() to detect changes 
 * in optional resources.
 *
 * @param name			Library name of a plugin or extension.
 * @return				True if exists, false otherwise.
 */
native bool:LibraryExists(const String:name[]);

/**
 * Returns the status of an extension, by filename.
 *
 * @param name			Extension name (like "sdktools.ext").
 * @param error			Optional error message buffer.
 * @param maxlength		Length of optional error message buffer.
 * @return				-2 if the extension was not found.
 *						-1 if the extension was found but failed to load.
 * 						 0 if the extension loaded but reported an error.
 * 						 1 if the extension is running without error.
 */
native GetExtensionFileStatus(const String:name[], String:error[]="", maxlength=0);

/**
 * Called after a library is added that the current plugin references 
 * optionally. A library is either a plugin name or extension name, as 
 * exposed via its include file.
 *
 * @param name			Library name.
 */
forward OnLibraryAdded(const String:name[]);

/**
 * Called right before a library is removed that the current plugin references 
 * optionally.  A library is either a plugin name or extension name, as 
 * exposed via its include file.
 *
 * @param name			Library name.
 */
forward OnLibraryRemoved(const String:name[]);

#include <helpers>
#include <entity>

