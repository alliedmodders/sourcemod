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

#if defined _core_included
 #endinput
#endif
#define _core_included

#include <version>

#define SOURCEMOD_PLUGINAPI_VERSION		2
struct PlVers
{
	version,
	String:filevers[],
};

/**
 * Specifies what to do after a hook completes.
 */
enum Action
{
	Plugin_Continue = 0,	/**< Continue with the original action */
	Plugin_Handled = 3,		/**< Handle the action at the end (don't call it) */
	Plugin_Stop = 4,		/**< Immediately stop the hook chain and handle the original */
}

public PlVers:__version = 
{
	version = SOURCEMOD_PLUGINAPI_VERSION,
	filevers = SOURCEMOD_VERSION
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

struct Extension
{
	const String:name[],	/* Short name */
	const String:file[],	/* Default file name */
	bool:autoload,			/* Whether or not to auto-load */
	bool:required,			/* Whether or not to require */
};

#define AUTOLOAD_EXTENSIONS
#define REQUIRE_EXTENSIONS
