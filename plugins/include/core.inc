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

#define SOURCEMOD_PLUGINAPI_VERSION		1
struct PlVers
{
	version,
};

public PlVers:__version = 
{
	version = SOURCEMOD_PLUGINAPI_VERSION,
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
