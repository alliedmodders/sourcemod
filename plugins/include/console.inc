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
 
#if defined _console_included
 #endinput
#endif
#define _console_included

/**
 * Flags for console commands and console variables
 * @note The descriptions for each constant come directly from the Source SDK.
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
 * Executes a server command as if it were on the server console (or RCON)
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native ServerCommand(const String:format[], {Handle,Float,String,_}:...);

/**
 * Inserts a server command at the beginning of the server command buffer.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native InsertServerCommand(const String:format[], {Handle,Float,String,_}:...);

/**
 * Executes every command in the server's command buffer, rather than once per frame.
 *
 * @noreturn
 */
native ServerExecute();

/**
 * Sends a message to the server console.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
native PrintToServer(const String:format[], {Handle,Float,String,_}:...);

/**
 * Sends a message to a client's console.
 *
 * @param client		Player index.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error			If the client is not connected an error will be thrown.
 */
native PrintToConsole(client, const String:format[], {Handle,Float,String,_}:...);

/**
 * Called when a server-only command is invoked.  
 *
 * @params args			Number of arguments that were in the argument string.
 * @return				A Result value.  Not handling the command
 *						means that Source will report it as "not found."
 */
functag SrvCmd Action:public(args);

/**
 * Creates a server-only console command, or hooks an already existing one.  
 *
 * @param cmd			Name of the command to hook or create.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param description	Optional description to use for command creation.
 * @param flags			Optional flags to use for command creation.
 * @noreturn
 */
native RegServerCmd(const String:cmd[], SrvCmd:callback, const String:description[]="", flags=0);

/**
 * Called when a generic console command is invoked.
 *
 * @param client		Index of the client, or 0 from the server.
 * @param args			Number of arguments that were in the argument string.
 * @return				A Result value.  Not handling the command
 *						means that Source will report it as "not found."
 */
functag ConCmd Action:public(client, args);

/**
 * Creates a console command, or hooks an already existing one.
 *
 * @param cmd			Name of the command to hook or create.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param description	Optional description to use for command creation.
 * @param flags			Optional flags to use for command creation.
 * @noreturn
 */
native RegConsoleCmd(const String:cmd[], ConCmd:callback, const String:description[]="", flags=0);

/**
 * Creates a console command as an administrative command.  If the command does not exist,
 * it is created.  When this command is invoked, the access rights of the player are 
 * automatically checked before allowing it to continue.
 *
 * @param cmd			String containing command to register.
 * @param callback		A function to use as a callback for when the command is invoked.
 * @param adminflags	Administrative flags (bitstring) to use for permissions.
 * @param group			String containing the command group to use.  If empty,
 * 						the plugin's filename will be used instead.
 * @param description	Optional description to use for help.
 * @param flags			Optional console flags.
 * @noreturn
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
 * @noreturn
 */
native GetCmdArg(argnum, String:buffer[], maxlength);

/**
 * Retrieves the entire command argument string in one lump from the current 
 * console or server command.
 *
 * @param buffer		Buffer to use for storing the string.
 * @param maxlength		Maximum length of the buffer.
 * @noreturn
 */
native GetCmdArgString(String:buffer[], maxlength);

/**
 * Creates a new console variable.
 *
 * @param name			Name of new convar.
 * @param defaultValue	String containing the default value of new convar.
 * @param helpText		Optional description of the convar.
 * @param flags			Optional bitstream of flags determining how the convar should be handled. (See FCVAR_* constants for more details)
 * @param hasMin		Optional boolean that determines if the convar has a minimum value.
 * @param min			Minimum floating point value that the convar can have if hasMin is true.
 * @param hasMax		Optional boolean that determines if the convar has a maximum value.
 * @param max			Maximum floating point value that the convar can have if hasMax is true.
 * @return				A handle to the newly created convar. If the convar already exists, INVALID_HANDLE is returned.
 * @error				Convar name is blank or is the same as a console command.
 */
native Handle:CreateConVar(const String:name[], const String:defaultValue[], const String:helpText[]="", flags=0, bool:hasMin=false, Float:min=0.0, bool:hasMax=false, Float:max=0.0);

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
functag OnConVarChanged public(Handle:convar, const String:oldValue[], const String:newValue[]);

/**
 * Creates a hook for when a console variable's value is changed.
 *
 * @param convar		Handle to the convar.
 * @param callback		An OnConVarChanged function pointer.
 * @noreturn
 * @error				Invalid or corrupt Handle or invalid callback function.
 */
native HookConVarChange(Handle:convar, OnConVarChanged:callback);

/**
 * Removes a hook for when a console variable's value is changed.
 *
 * @param convar		Handle to the convar.
 * @param callback		An OnConVarChanged function pointer.
 * @noreturn
 * @error				Invalid or corrupt Handle, invalid callback function, or no active hook on convar.
 */
native UnhookConVarChange(Handle:convar, OnConVarChanged:callback);

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
 * @param convar		Handle to the convar.
 * @param value			New boolean value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarBool(Handle:convar, bool:value);

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
 * @param convar		Handle to the convar.
 * @param value			New integer value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarInt(Handle:convar, value);

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
 * @param convar		Handle to the convar.
 * @param value			New floating point value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarFloat(Handle:convar, Float:value);

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
 * @param convar		Handle to the convar.
 * @param value			New string value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetConVarString(Handle:convar, const String:value[]);

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
 * Retrieves the name of a console variable.
 *
 * @param convar		Handle to the convar.
 * @param value			Buffer to store the name of the convar.
 * @param maxlength		Maximum length of string buffer.
 * @noreturn
 * @error				Invalid or corrupt Handle.     
 */
native GetConVarName(Handle:convar, const String:name[], maxlength);

/**
 * Retrieves the minimum floating point value that a console variable can contain.
 *
 * @param convar		Handle to the convar.
 * @param min			By-reference cell to store the minimum floating point value.
 * @return				True if the convar has a minimum value set, false otherwise.
 * @error				Invalid or corrupt Handle.
 */
native bool:GetConVarMin(Handle:convar, &Float:min);

/**
 * Retrieves the maximum floating point value that a console variable can contain.
 *
 * @param convar		Handle to the convar.
 * @param min			By-reference cell to store the maximum floating point value.
 * @return				True if the convar has a maximum value set, false otherwise.
 * @error				Invalid or corrupt Handle.
 */
native bool:GetConVarMax(Handle:convar, &Float:max);

/**
 * Resets the console variable to its default value.
 *
 * @param convar		Handle to the convar.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native ResetConVar(Handle:convar);
