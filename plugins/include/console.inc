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
 
#if defined _console_included
 #endinput
#endif
#define _console_included

#define INVALID_FCVAR_FLAGS		(-1)

/**
 * Console variable bound values used with Get/SetConVarBounds()
 */
enum ConVarBounds
{
	ConVarBound_Upper = 0,
	ConVarBound_Lower
};

/**
 * Console variable query helper values.
 */
enum QueryCookie
{
	QUERYCOOKIE_FAILED = 0,
};

/**
 * Reply sources for commands.
 */
enum ReplySource
{
	SM_REPLY_TO_CONSOLE = 0,
	SM_REPLY_TO_CHAT = 1,
};

/**
 * Console variable query result values.
 */
enum ConVarQueryResult
{
	ConVarQuery_Okay = 0,				/**< Retrieval of client convar value was successful. */
	ConVarQuery_NotFound,				/**< Client convar was not found. */
	ConVarQuery_NotValid,				/**< A console command with the same name was found, but there is no convar. */
	ConVarQuery_Protected				/**< Client convar was found, but it is protected. The server cannot retrieve its value. */
};

/**
 * @section Flags for console commands and console variables.  The descriptions 
 * for each constant come directly from the Source SDK.
 */
#define FCVAR_NONE				0		/**< The default, no flags at all */
#define FCVAR_UNREGISTERED		(1<<0)	/**< If this is set, don't add to linked list, etc. */
#define FCVAR_LAUNCHER			(1<<1)	/**< Defined by launcher. */
#define FCVAR_GAMEDLL			(1<<2)	/**< Defined by the game DLL. */
#define FCVAR_CLIENTDLL			(1<<3)	/**< Defined by the client DLL. */
#define FCVAR_MATERIAL_SYSTEM	(1<<4)	/**< Defined by the material system. */
#define FCVAR_PROTECTED			(1<<5)	/**< It's a server cvar, but we don't send the data since it's a password, etc. Sends 1 if it's not bland/zero, 0 otherwise as value. */
#define FCVAR_SPONLY			(1<<6)	/**< This cvar cannot be changed by clients connected to a multiplayer server. */
#define	FCVAR_ARCHIVE			(1<<7)	/**< Set to cause it to be saved to vars.rc */
#define	FCVAR_NOTIFY			(1<<8)	/**< Notifies players when changed. */
#define	FCVAR_USERINFO			(1<<9)	/**< Changes the client's info string. */
#define FCVAR_PRINTABLEONLY		(1<<10)	/**< This cvar's string cannot contain unprintable characters (e.g., used for player name, etc.) */
#define FCVAR_UNLOGGED			(1<<11)	/**< If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log */
#define FCVAR_NEVER_AS_STRING	(1<<12)	/**< Never try to print that cvar. */
#define FCVAR_REPLICATED		(1<<13)	/**< Server setting enforced on clients. */
#define FCVAR_CHEAT				(1<<14)	/**< Only useable in singleplayer / debug / multiplayer & sv_cheats */
#define FCVAR_STUDIORENDER		(1<<15)	/**< Defined by the studiorender system. */
#define FCVAR_DEMO				(1<<16)	/**< Record this cvar when starting a demo file. */
#define FCVAR_DONTRECORD		(1<<17)	/**< Don't record these command in demo files. */
#define FCVAR_PLUGIN			(1<<18)	/**< Defined by a 3rd party plugin. */
#define FCVAR_DATACACHE			(1<<19)	/**< Defined by the datacache system. */
#define FCVAR_TOOLSYSTEM		(1<<20)	/**< Defined by an IToolSystem library */
#define FCVAR_FILESYSTEM		(1<<21)	/**< Defined by the file system. */
#define FCVAR_NOT_CONNECTED		(1<<22)	/**< Cvar cannot be changed by a client that is connected to a server. */
#define FCVAR_SOUNDSYSTEM		(1<<23)	/**< Defined by the soundsystem library. */
#define FCVAR_ARCHIVE_XBOX		(1<<24)	/**< Cvar written to config.cfg on the Xbox. */
#define FCVAR_INPUTSYSTEM		(1<<25)	/**< Defined by the inputsystem DLL. */
#define FCVAR_NETWORKSYSTEM		(1<<26)	/**< Defined by the network system. */
#define FCVAR_VPHYSICS			(1<<27)	/**< Defined by vphysics. */

/**
 * @endsection
 */

/**
 * Executes a server command as if it were on the server console (or RCON)
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native ServerCommand(const String:format[], any:...);

/**
 * Inserts a server command at the beginning of the server command buffer.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native InsertServerCommand(const String:format[], any:...);

/**
 * Executes every command in the server's command buffer, rather than once per frame.
 *
 * @noreturn
 */
native ServerExecute();

/**
 * Executes a client command.  Note that this will not work on clients unless
 * they have cl_restrict_server_commands set to 0.
 *
 * @param client		Index of the client.
 * @param fmt			Format of the client command.
 * @param ...			Format parameters/
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native ClientCommand(client, const String:fmt[], any:...);

/**
 * Executes a client command on the server without being networked.
 *
 * FakeClientCommand() overwrites the command tokenization buffer.  This can 
 * cause undesired effects because future calls to GetCmdArg* will return 
 * data from the FakeClientCommand(), not the parent command.  If you are in 
 * a hook where this matters (for example, a "say" hook), you should use 
 * FakeClientCommandEx() instead.
 *
 * @param client		Index of the client.
 * @param fmt			Format of the client command.
 * @param ...			Format parameters
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native FakeClientCommand(client, const String:fmt[], any:...);

/**
 * Executes a client command on the server without being networked.  The 
 * execution of the client command is delayed by one frame to prevent any 
 * re-entrancy issues that might surface with FakeClientCommand().
 *
 * @param client		Index of the client.
 * @param fmt			Format of the client command.
 * @param ...			Format parameters
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native FakeClientCommandEx(client, const String:fmt[], any:...);

/**
 * Sends a message to the server console.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native PrintToServer(const String:format[], any:...);

/**
 * Sends a message to a client's console.
 *
 * @param client		Client index.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
native PrintToConsole(client, const String:format[], any:...);

/**
 * Reples to a message in a command.
 *
 * A client index of 0 will use PrintToServer().
 * If the command was from the console, PrintToConsole() is used.
 * If the command was from chat, PrintToChat() is used.
 *
 * @param client		Client index, or 0 for server.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				If the client is not connected or invalid.
 */
native ReplyToCommand(client, const String:format[], any:...);

/**
 * Returns the current reply source of a command.
 *
 * @return				ReplySource value.
 */
native ReplySource:GetCmdReplySource();

/**
 * Sets the current reply source of a command.
 *
 * Only use this if you know what you are doing.  You should save the old value
 * and restore it once you are done.
 *
 * @param source		New ReplySource value.
 * @return				Old ReplySource value.
 */
native ReplySource:SetCmdReplySource(ReplySource:source);

/**
 * Returns whether the current say hook is a chat trigger.
 *
 * This function is only meaningful inside say or say_team hooks.
 *
 * @return				True if a chat trigger, false otherwise.
 */
native bool:IsChatTrigger();

/**
 * Displays usage of an admin command to users depending on the 
 * setting of the sm_show_activity cvar.  All users receive a message 
 * in their chat text, except for the originating client, who receives 
 * the message based on the current ReplySource.
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param tag			Tag to prepend to the message.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error
 */
native ShowActivity2(client, const String:tag[], const String:format[], any:...);

/**
 * Displays usage of an admin command to users depending on the 
 * setting of the sm_show_activity cvar.  
 *
 * This version does not display a message to the originating client 
 * if used from chat triggers or menus.  If manual replies are used 
 * for these cases, then this function will suffice.  Otherwise, 
 * ShowActivity2() is slightly more useful.
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error
 */
native ShowActivity(client, const String:format[], any:...);

/**
 * Same as ShowActivity(), except the tag parameter is used instead of
 * "[SM] " (note that you must supply any spacing).
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param tag			Tag to display with.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error
 */
native ShowActivityEx(client, const String:tag[], const String:format[], any:...);

/**
 * Given an originating client and a target client, returns the string 
 * that describes the originating client according to the sm_show_activity cvar.
 *
 * For example, "ADMIN", "PLAYER", or a player's name could be placed in this buffer.
 *
 * @param client        Originating client; may be 0 for server console.
 * @param target        Targeted client.
 * @param namebuf       Name buffer.
 * @param maxlength     Maximum size of the name buffer.
 * @return              True if activity should be shown.  False otherwise.  In either 
 *                      case, the name buffer is filled.  The return value can be used 
 *                      to broadcast a "safe" name to all players regardless of the 
 *                      sm_show_activity filters.
 * @error               Invalid client index or client not connected.
 */
native FormatActivitySource(client, target, const String:namebuf[], maxlength);

/**
 * Called when a server-only command is invoked.  
 *
 * @params args			Number of arguments that were in the argument string.
 * @return				An Action value.  Not handling the command
 *						means that Source will report it as "not found."
 */
functag public Action:SrvCmd(args);

/**
 * Creates a server-only console command, or hooks an already existing one.  
 *
 * Server commands are case sensitive.
 *
 * @param cmd			Name of the command to hook or create.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param description	Optional description to use for command creation.
 * @param flags			Optional flags to use for command creation.
 * @noreturn
 * @error				Command name is the same as an existing convar.
 */
native RegServerCmd(const String:cmd[], SrvCmd:callback, const String:description[]="", flags=0);

/**
 * Called when a generic console command is invoked.
 *
 * @param client		Index of the client, or 0 from the server.
 * @param args			Number of arguments that were in the argument string.
 * @return				An Action value.  Not handling the command
 *						means that Source will report it as "not found."
 */
functag public Action:ConCmd(client, args);

/**
 * Creates a console command, or hooks an already existing one.
 *
 * Console commands are case sensitive.  However, if the command already exists in the game, 
 * the a client may enter the command in any case.  SourceMod corrects for this automatically, 
 * and you should only hook the "real" version of the command.
 *
 * @param cmd			Name of the command to hook or create.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param description	Optional description to use for command creation.
 * @param flags			Optional flags to use for command creation.
 * @noreturn
 * @error				Command name is the same as an existing convar.
 */
native RegConsoleCmd(const String:cmd[], ConCmd:callback, const String:description[]="", flags=0);

/**
 * Creates a console command as an administrative command.  If the command does not exist,
 * it is created.  When this command is invoked, the access rights of the player are 
 * automatically checked before allowing it to continue.
 *
 * Admin commands are case sensitive from both the client and server.
 *
 * @param cmd			String containing command to register.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param adminflags	Administrative flags (bitstring) to use for permissions.
 * @param description	Optional description to use for help.
 * @param group			String containing the command group to use.  If empty,
 * 						the plugin's filename will be used instead.
 * @param flags			Optional console flags.
 * @noreturn
 * @error				Command name is the same as an existing convar.
 */
native RegAdminCmd(const String:cmd[],
					ConCmd:callback,
					adminflags,
					const String:description[]="",
					const String:group[]="",
					flags=0);
					
/**
 * Returns the number of arguments from the current console or server command.
 * @note Unlike the HL2 engine call, this does not include the command itself.
 *
 * @return				Number of arguments to the current command.
 */
native GetCmdArgs();

/**
 * Retrieves a command argument given its index, from the current console or 
 * server command.
 * @note Argument indexes start at 1; 0 retrieves the command name.
 *
 * @param argnum		Argument number to retrieve.
 * @param buffer		Buffer to use for storing the string.
 * @param maxlength		Maximum length of the buffer.
 * @return				Length of string written to buffer.
 */
native GetCmdArg(argnum, String:buffer[], maxlength);

/**
 * Retrieves the entire command argument string in one lump from the current 
 * console or server command.
 *
 * @param buffer		Buffer to use for storing the string.
 * @param maxlength		Maximum length of the buffer.
 * @return				Length of string written to buffer.
 */
native GetCmdArgString(String:buffer[], maxlength);

/**
 * Creates a new console variable.
 *
 * @param name			Name of new convar.
 * @param defaultValue	String containing the default value of new convar.
 * @param description	Optional description of the convar.
 * @param flags			Optional bitstring of flags determining how the convar should be handled. See FCVAR_* constants for more details.
 * @param hasMin		Optional boolean that determines if the convar has a minimum value.
 * @param min			Minimum floating point value that the convar can have if hasMin is true.
 * @param hasMax		Optional boolean that determines if the convar has a maximum value.
 * @param max			Maximum floating point value that the convar can have if hasMax is true.
 * @return				A handle to the newly created convar. If the convar already exists, a handle to it will still be returned.
 * @error				Convar name is blank or is the same as an existing console command.
 */
native Handle:CreateConVar(const String:name[], const String:defaultValue[], const String:description[]="", flags=0, bool:hasMin=false, Float:min=0.0, bool:hasMax=false, Float:max=0.0);

/**
 * Searches for a console variable.
 *
 * @param name			Name of convar to find.
 * @return				A handle to the convar if it is found. INVALID_HANDLE otherwise.
 */
native Handle:FindConVar(const String:name[]);

/**
 * Called when a console variable's value is changed.
 *
 * @param convar		Handle to the convar that was changed.
 * @param oldValue		String containing the value of the convar before it was changed.
 * @param newValue		String containing the new value of the convar.
 * @noreturn
 */
functag public ConVarChanged(Handle:convar, const String:oldValue[], const String:newValue[]);

/**
 * Creates a hook for when a console variable's value is changed.
 *
 * @param convar		Handle to the convar.
 * @param callback		An OnConVarChanged function pointer.
 * @noreturn
 * @error				Invalid or corrupt Handle or invalid callback function.
 */
native HookConVarChange(Handle:convar, ConVarChanged:callback);

/**
 * Removes a hook for when a console variable's value is changed.
 *
 * @param convar		Handle to the convar.
 * @param callback		An OnConVarChanged function pointer.
 * @noreturn
 * @error				Invalid or corrupt Handle, invalid callback function, or no active hook on convar.
 */
native UnhookConVarChange(Handle:convar, ConVarChanged:callback);

/**
 * Returns the boolean value of a console variable.
 *
 * @param convar		Handle to the convar.
 * @return				The boolean value of the convar.
 * @error				Invalid or corrupt Handle.
 */
native bool:GetConVarBool(Handle:convar);

/**
 * Sets the boolean value of a console variable.
 *
 * Note: The replicate and notify params are ignored on the engines for Episode 2/Orange Box
 * and Left 4 Dead. These engines automatically replicates and notifies as soon as the convar
 * is changed.
 *
 * @param convar		Handle to the convar.
 * @param value			New boolean value.
 * @param replicate		If set to true, the new convar value will be set on all clients.
 *						This will only work if the convar has the FCVAR_REPLICATED flag
 *						and actually exists on clients.
 * @param notify		If set to true, clients will be notified that the convar has changed.
 *						This will only work if the convar has the FCVAR_NOTIFY flag.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarBool(Handle:convar, bool:value, bool:replicate=false, bool:notify=false);

/**
 * Returns the integer value of a console variable.
 *
 * @param convar		Handle to the convar.
 * @return				The integer value of the convar.
 * @error				Invalid or corrupt Handle.
 */
native GetConVarInt(Handle:convar);

/**
 * Sets the integer value of a console variable.
 *
 * Note: The replicate and notify params are ignored on the engines for Episode 2/Orange Box
 * and Left 4 Dead. These engines automatically replicates and notifies as soon as the convar
 * is changed.
 *
 * @param convar		Handle to the convar.
 * @param value			New integer value.
 * @param replicate		If set to true, the new convar value will be set on all clients.
 *						This will only work if the convar has the FCVAR_REPLICATED flag
 *						and actually exists on clients.
 * @param notify		If set to true, clients will be notified that the convar has changed.
 *						This will only work if the convar has the FCVAR_NOTIFY flag.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarInt(Handle:convar, value, bool:replicate=false, bool:notify=false);

/**
 * Returns the floating point value of a console variable.
 *
 * @param convar		Handle to the convar.
 * @return				The floating point value of the convar.
 * @error				Invalid or corrupt Handle.
 */
native Float:GetConVarFloat(Handle:convar);

/**
 * Sets the floating point value of a console variable.
 *
 * Note: The replicate and notify params are ignored on the engines for Episode 2/Orange Box
 * and Left 4 Dead. These engines automatically replicates and notifies as soon as the convar
 * is changed.
 *
 * @param convar		Handle to the convar.
 * @param value			New floating point value.
 * @param replicate		If set to true, the new convar value will be set on all clients.
 *						This will only work if the convar has the FCVAR_REPLICATED flag
 *						and actually exists on clients.
 * @param notify		If set to true, clients will be notified that the convar has changed.
 *						This will only work if the convar has the FCVAR_NOTIFY flag.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarFloat(Handle:convar, Float:value, bool:replicate=false, bool:notify=false);

/**
 * Retrieves the string value of a console variable.
 *
 * @param convar		Handle to the convar.
 * @param value			Buffer to store the value of the convar.
 * @param maxlength		Maximum length of string buffer.
 * @noreturn
 * @error				Invalid or corrupt Handle.     
 */
native GetConVarString(Handle:convar, String:value[], maxlength);

/**
 * Sets the string value of a console variable.
 *
 * Note: The replicate and notify params are ignored on the engines for Episode 2/Orange Box
 * and Left 4 Dead. These engines automatically replicates and notifies as soon as the convar
 * is changed.
 *
 * @param convar		Handle to the convar.
 * @param value			New string value.
 * @param replicate		If set to true, the new convar value will be set on all clients.
 *						This will only work if the convar has the FCVAR_REPLICATED flag
 *						and actually exists on clients.
 * @param notify		If set to true, clients will be notified that the convar has changed.
 *						This will only work if the convar has the FCVAR_NOTIFY flag.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarString(Handle:convar, const String:value[], bool:replicate=false, bool:notify=false);

/**
 * Resets the console variable to its default value.
 *
 * Note: The replicate and notify params are ignored on the engines for Episode 2/Orange Box
 * and Left 4 Dead. These engines automatically replicates and notifies as soon as the convar
 * is changed.
 *
 * @param convar		Handle to the convar.
 * @param replicate		If set to true, the new convar value will be set on all clients.
 *						This will only work if the convar has the FCVAR_REPLICATED flag
 *						and actually exists on clients.
 * @param notify		If set to true, clients will be notified that the convar has changed.
 *						This will only work if the convar has the FCVAR_NOTIFY flag.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native ResetConVar(Handle:convar, bool:replicate=false, bool:notify=false);

/**
 * Returns the bitstring of flags on a console variable.
 *
 * @param convar		Handle to the convar.
 * @return				A bitstring containing the FCVAR_* flags that are enabled.
 * @error				Invalid or corrupt Handle.
 */
native GetConVarFlags(Handle:convar);

/**
 * Sets the bitstring of flags on a console variable.
 *
 * @param convar		Handle to the convar.
 * @param flags			A bitstring containing the FCVAR_* flags to enable.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarFlags(Handle:convar, flags);

/**
 * Retrieves the specified bound of a console variable.
 *
 * @param convar		Handle to the convar.
 * @param type			Type of bound to retrieve, ConVarBound_Lower or ConVarBound_Upper.
 * @param value			By-reference cell to store the specified floating point bound value.
 * @return				True if the convar has the specified bound set, false otherwise.
 * @error				Invalid or corrupt Handle.
 */
native bool:GetConVarBounds(Handle:convar, ConVarBounds:type, &Float:value);

/**
 * Sets the specified bound of a console variable.
 *
 * @param convar		Handle to the convar.
 * @param type			Type of bound to set, ConVarBound_Lower or ConVarBound_Upper
 * @param set			If set to true, convar will use specified bound. If false, bound will be removed.
 * @param value			Floating point value to use as the specified bound.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarBounds(Handle:convar, ConVarBounds:type, bool:set, Float:value=0.0);

/**
 * Retrieves the name of a console variable.
 *
 * @param convar		Handle to the convar.
 * @param value			Buffer to store the name of the convar.
 * @param maxlength		Maximum length of string buffer.
 * @noreturn
 * @error				Invalid or corrupt Handle.     
 */
native GetConVarName(Handle:convar, const String:name[], maxlength);

funcenum ConVarQueryFinished
{	
	/**
	 * Called when a query to retrieve a client's console variable has finished.
	 *
	 * @param cookie		Unique identifier of query.
	 * @param client		Player index.
	 * @param result		Result of query that tells one whether or not query was successful.
	 *						See ConVarQueryResult enum for more details.
	 * @param convarName	Name of client convar that was queried.
	 * @param convarValue	Value of client convar that was queried if successful. This will be "" if it was not.
	 * @param value			Value that was passed when query was started.
	 * @noreturn
	 */
	public(QueryCookie:cookie, client, ConVarQueryResult:result, const String:cvarName[], const String:cvarValue[], any:value),
	
	/**
	 * Called when a query to retrieve a client's console variable has finished.
	 *
	 * @param cookie		Unique identifier of query.
	 * @param client		Player index.
	 * @param result		Result of query that tells one whether or not query was successful.
	 *						See ConVarQueryResult enum for more details.
	 * @param convarName	Name of client convar that was queried.
	 * @param convarValue	Value of client convar that was queried if successful. This will be "" if it was not.
	 * @noreturn
	 */
	public(QueryCookie:cookie, client, ConVarQueryResult:result, const String:cvarName[], const String:cvarValue[])
};

/**
 * Starts a query to retrieve the value of a client's console variable.
 *
 * @param client		Player index.
 * @param name			Name of client convar to query.
 * @param callback		A function to use as a callback when the query has finished.
 * @param value			Optional value to pass to the callback function.
 * @return				A cookie that uniquely identifies the query. 
 *						Returns QUERYCOOKIE_FAILED on failure, such as when used on a bot.
 */
native QueryCookie:QueryClientConVar(client, const String:cvarName[], ConVarQueryFinished:callback, any:value=0);

/**
 * Gets a command iterator.  Must be freed with CloseHandle().
 *
 * @return				A new command iterator.
 */
native Handle:GetCommandIterator();

/**
 * Reads a command iterator, then advances to the next command if any.
 * Only SourceMod specific commands are returned.
 *
 * @param iter			Command iterator Handle.
 * @param name			Name buffer.
 * @param nameLen		Name buffer size.
 * @param eflags		Effective default flags of a command.
 * @param desc			Command description buffer.
 * @param descLen		Command description buffer size.
 * @return				True on success, false if there are no more commands.
 */
native bool:ReadCommandIterator(Handle:iter, 
								String:name[], 
								nameLen, 
								&eflags=0, 
								String:desc[]="", 
								descLen=0);

/**
 * Returns whether a client has access to a given command string.  The string 
 * can also be any override string, as overrides can be independent of 
 * commands.  This important feature essentially allows you to create custom 
 * flags using the override system.
 *
 * @param client		Client index.
 * @param command		Command name.  If the command is not found, the default 
 *						flags are used.
 * @param flags			Flag string to use as a default, if the command or override 
 *						is not found.
 * @param override_only	If true, SourceMod will not attempt to find a matching 
 *						command, and it will only use the default flags specified.
 *						Otherwise, SourceMod will ignore the default flags if 
 *						there is a matching admin command.
 * @return				True if the client has access, false otherwise.
 */
native bool:CheckCommandAccess(client, 
							   const String:command[],
							   flags,
							   bool:override_only=false);

/**
 * Returns true if the supplied character is valid in a ConVar name.
 *
 * @param c			Character to validate.
 * @return			True is valid for ConVars, false otherwise
 */
stock bool:IsValidConVarChar(c)
{
	return (c == '_' || IsCharAlpha(c) || IsCharNumeric(c));
}

/**
 * Returns the bitstring of flags of a command.
 *
 * @param name		Name of the command.
 * @return			A bitstring containing the FCVAR_* flags that are enabled 
 *					or INVALID_FCVAR_FLAGS if command not found.
 */
native GetCommandFlags(const String:name[]);

/**
 * Sets the bitstring of flags of a command.
 *
 * @param name		Name of the command.
 * @param flags		A bitstring containing the FCVAR_* flags to enable.
 * @return			True on success, otherwise false.
 */
native bool:SetCommandFlags(const String:name[], flags);

/**
 * Starts a ConCommandBase search, traversing the list of ConVars and 
 * ConCommands.  If a Handle is returned, the next entry must be read 
 * via FindNextConCommand().  The order of the list is undefined.
 *
 * @param buffer		Buffer to store entry name.
 * @param max_size		Maximum size of the buffer.
 * @param isCommand		Variable to store whether the entry is a command. 
 *						If it is not a command, it is a ConVar.
 * @param flags			Variable to store entry flags.
 * @param description		Buffer to store the description, empty if no description present.
 * @param descrmax_size		Maximum size of the description buffer.
 * @return				On success, a ConCmdIter Handle is returned, which 
 *						can be read via FindNextConCommand(), and must be 
 *						closed via CloseHandle().  Additionally, the output 
 *						parameters will be filled with information of the 
 *						first ConCommandBase entry.
 *						On failure, INVALID_HANDLE is returned, and the 
 *						contents of outputs is undefined.
 */
native Handle:FindFirstConCommand(String:buffer[], max_size, &bool:isCommand, &flags=0, String:description[]="", descrmax_size=0);

/**
 * Reads the next entry in a ConCommandBase iterator.
 *
 * @param search		ConCmdIter Handle to search.
 * @param buffer		Buffer to store entry name.
 * @param max_size		Maximum size of the buffer.
 * @param isCommand		Variable to store whether the entry is a command.
 *						If it is not a command, it is a ConVar.
 * @param flags			Variable to store entry flags.
 * @param description		Buffer to store the description, empty if no description present.
 * @param descrmax_size		Maximum size of the description buffer.
 * @return				On success, the outputs are filled, the iterator is 
 *						advanced to the next entry, and true is returned.  
 *						If no more entries exist, false is returned, and the 
 *						contents of outputs is undefined.
 */
native bool:FindNextConCommand(Handle:search, String:buffer[], max_size, &bool:isCommand, &flags=0, String:description[]="", descrmax_size=0);

/**
 * Replicates a convar value to a specific client. This does not change the actual convar value.
 *
 * @param client		Client index
 * @param convar		ConVar handle
 * @param value			String value to send
 * @return				True on success, false on failure
 * @error 				Invalid client index, client not in game, or client is fake
 */
native bool:SendConVarValue(client, Handle:convar, const String:value[]);

/**
 * Adds an informational string to the server's public "tags".
 * This string should be a short, unique identifier.
 *
 * Note: Tags are automatically removed when a plugin unloads.
 * Note: Currently, this function does nothing because of bugs in the Valve master.
 *
 * @param tag			Tag string to append.
 * @noreturn
 */
native AddServerTag(const String:tag[]);

/**
 * Removes a tag previously added by the calling plugin.
 *
 * @param tag			Tag string to remove.
 * @noreturn
 */
native RemoveServerTag(const String:tag[]);

/**
 * Callback for command listeners. This is invoked whenever any command
 * reaches the server, from the server console itself or a player.

 * Returning Plugin_Handled or Plugin_Stop will prevent the original,
 * baseline code from running.
 *
 * -- TEXT BELOW IS IMPLEMENTATION, AND NOT GUARANTEED --
 * Even if returning Plugin_Handled or Plugin_Stop, some callbacks will still
 * trigger. These are:
 *  * C++ command dispatch hooks from Metamod:Source plugins
 *  * Reg*Cmd() hooks that did not create new commands.
 *
 * @param client        Client, or 0 for server. Client will be connected but
 *                      not necessarily in game.
 * @param command       Command name, lower case. To get name as typed, use
 *                      GetCmdArg() and specify argument 0.
 * @param argc          Argument count.
 * @return				Action to take (see extended notes above).
 */
functag public Action:CommandListener(client, const String:command[], argc);

#define FEATURECAP_COMMANDLISTENER  "command listener"

/**
 * Adds a callback that will fire when a command is sent to the server.
 *
 * Registering commands is designed to create a new command as part of the UI,
 * whereas this is a lightweight hook on a command string, existing or not.
 * Using Reg*Cmd to intercept is in poor practice, as it physically creates a
 * new command and can slow down dispatch in general.
 *
 * To see if this feature is available, use FeatureType_Capability and 
 * FEATURECAP_COMMANDLISTENER.
 *
 * @param callback      Callback.
 * @param command		Command, or if not specified, a global listener.
 *                      The command is case insensitive.
 * @return              True if this feature is available on the current game,
 *                      false otherwise.
 */
native bool:AddCommandListener(CommandListener:callback, const String:command[]="");

/**
 * Removes a previously added command listener, in reverse order of being added.
 *
 * @param callback		Callback.
 * @param command		Command, or if not specified, a global listener.
 *                      The command is case insensitive.
 * @error				Callback has no active listeners.
 */
native RemoveCommandListener(CommandListener:callback, const String:command[]="");

