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
 
#if defined _helpers_included
 #endinput
#endif
#define _helpers_included

/**
 * Formats a user's info as log text.
 *
 * @param client		Client index.
 * @param buffer		Buffer for text.
 * @param maxlength		Maximum length of text.
 */
stock FormatUserLogText(client, String:buffer[], maxlength)
{
	decl String:auth[32];
	decl String:name[40];
	
	new userid = GetClientUserId(client);
	if (!GetClientAuthString(client, auth, sizeof(auth)))
	{
		StrCopy(auth, sizeof(auth), "UNKNOWN");
	}
	if (!GetClientName(client, name, sizeof(name)))
	{
		StrCopy(name, sizeof(name), "UNKNOWN");
	}
	
	/** Currently, no team stuff ... */
	
	Format(buffer, maxlength, "\"%s<%d><%s><>\"", name, userid, auth);
}

/**
 * Returns plugin handle from plugin filename.
 *
 * @param filename		Filename of the plugin to search for.
 * @return				Handle to plugin if found, INVALID_HANDLE otherwise.
 */
stock Handle:FindPluginByFile(const String:filename[])
{
	decl String:buffer[256];
	
	new Handle:iter = GetPluginIterator();
	new Handle:pl;
	
	while (MorePlugins(iter))
	{
		pl = ReadPlugin(iter);
		
		GetPluginFilename(pl, buffer, sizeof(buffer));
		if (StrCompare(buffer, filename) == 0)
		{
			return pl;
		}
	}
	
	return INVALID_HANDLE;
}
