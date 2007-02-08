/**
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
#define FCVAR_NONE              0       /**< The default, no flags at all */
#define FCVAR_UNREGISTERED      (1<<0)  /**< If this is set, don't add to linked list, etc. */
#define FCVAR_LAUNCHER          (1<<1)  /**< Defined by launcher. */
#define FCVAR_GAMEDLL           (1<<2)  /**< Defined by the game DLL. */
#define FCVAR_CLIENTDLL         (1<<3)  /**< Defined by the client DLL. */
#define FCVAR_MATERIAL_SYSTEM   (1<<4)  /**< Defined by the material system. */
#define FCVAR_PROTECTED         (1<<5)  /**< It's a server cvar, but we don't send the data since it's a password, etc. Sends 1 if it's not bland/zero, 0 otherwise as value. */
#define FCVAR_SPONLY            (1<<6)  /**< This cvar cannot be changed by clients connected to a multiplayer server. */
#define	FCVAR_ARCHIVE           (1<<7)  /**< Set to cause it to be saved to vars.rc */
#define	FCVAR_NOTIFY            (1<<8)  /**< Notifies players when changed. */
#define	FCVAR_USERINFO          (1<<9)  /**< Changes the client's info string. */
#define FCVAR_PRINTABLEONLY     (1<<10) /**< This cvar's string cannot contain unprintable characters (e.g., used for player name, etc.) */
#define FCVAR_UNLOGGED          (1<<11) /**< If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log */
#define FCVAR_NEVER_AS_STRING   (1<<12) /**< Never try to print that cvar. */
#define FCVAR_REPLICATED        (1<<13) /**< Server setting enforced on clients. */
#define FCVAR_CHEAT             (1<<14) /**< Only useable in singleplayer / debug / multiplayer & sv_cheats */
#define FCVAR_STUDIORENDER      (1<<15) /**< Defined by the studiorender system. */
#define FCVAR_DEMO              (1<<16) /**< Record this cvar when starting a demo file. */
#define FCVAR_DONTRECORD        (1<<17) /**< Don't record these command in demo files. */
#define FCVAR_PLUGIN            (1<<18) /**< Defined by a 3rd party plugin. */
#define FCVAR_DATACACHE         (1<<19) /**< Defined by the datacache system. */
#define FCVAR_TOOLSYSTEM        (1<<20) /**< Defined by an IToolSystem library */
#define FCVAR_FILESYSTEM        (1<<21) /**< Defined by the file system. */
#define FCVAR_NOT_CONNECTED     (1<<22) /**< Cvar cannot be changed by a client that is connected to a server. */
#define FCVAR_SOUNDSYSTEM       (1<<23) /**< Defined by the soundsystem library. */
#define FCVAR_ARCHIVE_XBOX      (1<<24) /**< Cvar written to config.cfg on the Xbox. */
#define FCVAR_INPUTSYSTEM       (1<<25) /**< Defined by the inputsystem DLL. */
#define FCVAR_NETWORKSYSTEM     (1<<26) /**< Defined by the network system. */
#define FCVAR_VPHYSICS          (1<<27) /**< Defined by vphysics. */

/**
 * Creates a new console variable.
 *
 * @param name          Name of new convar.
 * @param defaultValue  String containing the default value of new convar.
 * @param helpText      Optional description of the convar.
 * @param flags         Optional bitstream of flags determining how the convar should be handled. (See FCVAR_* constants for more details)
 * @param hasMin        Optional boolean that determines if the convar has a minimum value.
 * @param min           Minimum floating point value that the convar can have if hasMin is true.
 * @param hasMax        Optional boolean that determines if the convar has a maximum value.
 * @param max           Maximum floating point value that the convar can have if hasMax is true.
 * @return				A handle to the newly created convar. If the convar already exists, INVALID_HANDLE is returned.
 */
native Handle:CreateConVar(const String:name[], const String:defaultValue[], const String:helpText[]="", flags=0, bool:hasMin=false, Float:min=0.0, bool:hasMax=false, Float:max=0.0);

/**
 * Searches for a console variable.
 *
 * @param name          Name of convar to find.
 * @return              A handle to the convar if it is found. INVALID_HANDLE otherwise.
 */
native Handle:FindConVar(const String:name[]);

/**
 * Returns the boolean value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @return              The boolean value of the convar.
 * @error               Invalid or corrupt Handle.
 */
native bool:GetConVarBool(Handle:convar);

/**
 * Sets the boolean value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @param value         New boolean value.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native SetConVarBool(Handle:convar, bool:value);

/**
 * Returns the integer value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @return              The integer value of the convar.
 * @error               Invalid or corrupt Handle.
 */
native GetConVarInt(Handle:convar);

/**
 * Sets the integer value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @param value         New integer value.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native SetConVarInt(Handle:convar, value);

/**
 * Returns the floating point value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @return              The floating point value of the convar.
 * @error               Invalid or corrupt Handle.
 */
native Float:GetConVarFloat(Handle:convar);

/**
 * Sets the floating point value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @param value         New floating point value.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native SetConVarFloat(Handle:convar, Float:value);

/**
 * Retrieves the string value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @param value         Buffer to store the value of the convar.
 * @param maxlen        Maximum length of string buffer.
 * @noreturn
 * @error               Invalid or corrupt Handle.     
 */
native GetConVarString(Handle:convar, String:value[], maxlen);

/**
 * Sets the string value of a console variable.
 *
 * @param convar        Handle to the convar.
 * @param value         New string value.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native SetConVarString(Handle:convar, const String:value[]);

/**
 * Returns the bitstring of flags on a console variable.
 *
 * @param convar        Handle to the convar.
 * @return              A bitstring containing the FCVAR_* flags that are enabled.
 * @error               Invalid or corrupt Handle.
 */
native GetConVarFlags(Handle:convar);

/**
 * Sets the bitstring of flags on a console variable.
 *
 * @param convar        Handle to the convar.
 * @param flags         A bitstring containing the FCVAR_* flags to enable.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native SetConVarFlags(Handle:convar, flags);

/**
 * Retrieves the minimum floating point value that a console variable can contain.
 *
 * @param convar        Handle to the convar.
 * @param min           By-reference cell to store the minimum floating point value.
 * @return              True if the convar has a minimum value set, false otherwise.
 * @error               Invalid or corrupt Handle.
 */
native bool:GetConVarMin(Handle:convar, &Float:min);

/**
 * Retrieves the maximum floating point value that a console variable can contain.
 *
 * @param convar        Handle to the convar.
 * @param min           By-reference cell to store the maximum floating point value.
 * @return              True if the convar has a maximum value set, false otherwise.
 * @error               Invalid or corrupt Handle.
 */
native bool:GetConVarMax(Handle:convar, &Float:max);

/**
 * Resets the console variable to its default value.
 *
 * @param convar        Handle to the convar.
 * @noreturn
 * @error               Invalid or corrupt Handle.
 */
native ResetConVar(Handle:convar);
