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
 
#if defined _sm_logging_included
 #endinput
#endif
#define _sm_logging_included

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
 * Called when a game log message is received.
 *
 * Any Log*() functions called within this callback will not recursively 
 * pass through.  That is, they will log directly, bypassing this callback.
 *
 * Note that this does not capture log messages from the engine.  It only 
 * captures log messages being sent from the game/mod itself.
 *
 * @param message		Message contents.
 * @return				Plugin_Handled or Plugin_Stop will prevent the message 
 * 						from being written to the log file.
 */
functag public Action:GameLogHook(const String:message[]);

/**
 * Adds a game log hook.
 *
 * @param hook			Hook function.
 * @noreturn
 */
native AddGameLogHook(GameLogHook:hook);

/**
 * Removes a game log hook.
 *
 * @param hook			Hook function.
 * @noreturn
 */
native RemoveGameLogHook(GameLogHook:hook);
