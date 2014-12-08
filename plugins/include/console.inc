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
 * Executes a server command as if it were on the server console (or RCON) 
 * and stores the printed text into buffer.
 *
 * Warning: This calls ServerExecute internally and may have issues if
 * certain commands are in the buffer, only use when you really need
 * the response.
 * Also, on L4D2 this will not print the command output to the server console.
 *
 * @param buffer		String to store command result into.
 * @param maxlen		Length of buffer.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native ServerCommandEx(String:buffer[], maxlen, const String:format[], any:...);

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
 * @param ...			Format parameters
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
 * @param args			Number of arguments that were in the argument string.
 * @return				An Action value.  Not handling the command
 *						means that Source will report it as "not found."
 */
typedef SrvCmd = function Action (int args);

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
typedef ConCmd = function Action (int client, int args);

/**
 * Creates a console command, or hooks an already existing one.
 *
 * Console commands are case sensitive.  However, if the command already exists in the game, 
 * a client may enter the command in any case.  SourceMod corrects for this automatically, 
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
 * can be any override string, as overrides can be independent of 
 * commands.  This feature essentially allows you to create custom 
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
 * Returns whether an admin has access to a given command string.  The string 
 * can be any override string, as overrides can be independent of 
 * commands.  This feature essentially allows you to create custom flags
 * using the override system.
 *
 * @param id			AdminId of the admin.
 * @param command		Command name.  If the command is not found, the default 
 *						flags are used.
 * @param flags			Flag string to use as a default, if the command or override 
 *						is not found.
 * @param override_only	If true, SourceMod will not attempt to find a matching 
 *						command, and it will only use the default flags specified.
 *						Otherwise, SourceMod will ignore the default flags if 
 *						there is a matching admin command.
 * @return				True if the admin has access, false otherwise.
 */
native bool:CheckAccess(AdminId:id, 
							   const String:command[],
							   flags,
							   bool:override_only=false);

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
 *
 * Clients may be in the process of connecting when they are executing commands
 * IsClientConnected(client) is not guaranteed to return true.  Other functions
 * such as GetClientIP() may not work at this point either.
 *
 * Returning Plugin_Handled or Plugin_Stop will prevent the original,
 * baseline code from running.
 *
 * -- TEXT BELOW IS IMPLEMENTATION, AND NOT GUARANTEED --
 * Even if returning Plugin_Handled or Plugin_Stop, some callbacks will still
 * trigger. These are:
 *  * C++ command dispatch hooks from Metamod:Source plugins
 *  * Reg*Cmd() hooks that did not create new commands.
 *
 * @param client        Client, or 0 for server.
 *                      Client may not be connected or in game.
 * @param command       Command name, lower case. To get name as typed, use
 *                      GetCmdArg() and specify argument 0.
 * @param argc          Argument count.
 * @return				Action to take (see extended notes above).
 */
typedef CommandListener = function Action (int client, const char[] command, int argc);

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

/**
 * Returns true if the supplied command exists.
 *
 * @param command	Command to find.
 * @return			True if command is found, false otherwise.
 */
stock bool:CommandExists(const String:command[])
{
	return (GetCommandFlags(command) != INVALID_FCVAR_FLAGS);
}
/**
 * Global listener for the chat commands.
 *
 * @param client		Client index.
 * @param command		Command name.
 * @param sArgs			Chat argument string.
 * 
 * @return				An Action value. Returning Plugin_Handled bypasses the game function call.
							Returning Plugin_Stop bypasses the post hook as well as the game function.
 */
forward Action:OnClientSayCommand(client, const String:command[], const String:sArgs[]);

/**
 * Global post listener for the chat commands.
 *
 * @param client		Client index.
 * @param command		Command name.
 * @param sArgs			Chat argument string.
 * 
 */
forward void OnClientSayCommand_Post(int client, const char[] command, const char[] sArgs);
