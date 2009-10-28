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

#if defined _core_included
 #endinput
#endif
#define _core_included

#include <version>

/** If this gets changed, you need to update Core's check. */
#define SOURCEMOD_PLUGINAPI_VERSION		5

struct PlVers
{
	version,
	String:filevers[],
	String:date[],
	String:time[]
};

/**
 * Function helper values.
 */
enum Function
{
     INVALID_FUNCTION = -1,
};

/**
 * Specifies what to do after a hook completes.
 */
enum Action
{
	Plugin_Continue = 0,	/**< Continue with the original action */
	Plugin_Changed = 1,		/**< Inputs or outputs have been overridden with new values */
	Plugin_Handled = 3,		/**< Handle the action at the end (don't call it) */
	Plugin_Stop = 4,		/**< Immediately stop the hook chain and handle the original */
};

/**
 * Specifies identity types.
 */
enum Identity
{
	Identity_Core = 0,
	Identity_Extension = 1,
	Identity_Plugin = 2
};

public PlVers:__version = 
{
	version = SOURCEMOD_PLUGINAPI_VERSION,
	filevers = SOURCEMOD_VERSION,
	date = __DATE__,
	time = __TIME__
};

/**
 * Plugin status values.
 */
enum PluginStatus
{
	Plugin_Running=0,		/**< Plugin is running */
	/* All states below are "temporarily" unexecutable */
	Plugin_Paused,			/**< Plugin is loaded but paused */
	Plugin_Error,			/**< Plugin is loaded but errored/locked */
	/* All states below do not have all natives */
	Plugin_Loaded,			/**< Plugin has passed loading and can be finalized */
	Plugin_Failed,			/**< Plugin has a fatal failure */
	Plugin_Created,			/**< Plugin is created but not initialized */
	Plugin_Uncompiled,		/**< Plugin is not yet compiled by the JIT */
	Plugin_BadLoad,			/**< Plugin failed to load */
};

/**
 * Plugin information properties.
 */
enum PluginInfo
{
	PlInfo_Name,			/**< Plugin name */
	PlInfo_Author,			/**< Plugin author */
	PlInfo_Description,		/**< Plugin description */
	PlInfo_Version,			/**< Plugin verison */
	PlInfo_URL,				/**< Plugin URL */
};

/**
 * Defines how an extension must expose itself for autoloading.
 */
struct Extension
{
	const String:name[],	/**< Short name */
	const String:file[],	/**< Default file name */
	bool:autoload,			/**< Whether or not to auto-load */
	bool:required,			/**< Whether or not to require */
};

/**
 * Defines how a plugin must expose itself for native requiring.
 */
struct SharedPlugin
{
	const String:name[],	/**< Short name */
	const String:file[],	/**< File name */
	bool:required,			/**< Whether or not to require */
};

public Float:NULL_VECTOR[3];		/**< Pass this into certain functions to act as a C++ NULL */
public const String:NULL_STRING[1];	/**< pass this into certain functions to act as a C++ NULL */

/**
 * Horrible compatibility shim.
 */
public Extension:__ext_core = 
{
	name = "Core",
	file = "core",
	autoload = 0,
	required = 0,
};

native VerifyCoreVersion();

/**
 * Sets a native as optional, such that if it is unloaded, removed,
 * or otherwise non-existent, the plugin will still work.  Calling
 * removed natives results in a run-time error.
 *
 * @param name			Native name.
 * @noreturn
 */
native MarkNativeAsOptional(const String:name[]);

public __ext_core_SetNTVOptional()
{
	MarkNativeAsOptional("GetFeatureStatus");
	MarkNativeAsOptional("RequireFeature");
	MarkNativeAsOptional("AddCommandListener");
	MarkNativeAsOptional("RemoveCommandListener");
	VerifyCoreVersion();
}


#define AUTOLOAD_EXTENSIONS
#define REQUIRE_EXTENSIONS
#define REQUIRE_PLUGIN
