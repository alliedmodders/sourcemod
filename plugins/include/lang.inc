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

#if defined _lang_included
 #endinput
#endif
#define _lang_included

#define LANG_SERVER		0		/**< Translate using the server's language */

/**
 * Loads a translation file for the plugin calling this native.
 * If no extension is specified, .txt is assumed.
 *
 * @param path		Translation file.
 * @noreturn
 */
native LoadTranslations(const String:file[]);

/**
 * Sets the global language target.  This is useful for creating functions
 * that will be compatible with the %t format specifier.  Note that invalid
 * indexes can be specified but the error will occur during translation,
 * not during this function call.
 *
 * @param client	Client index or LANG_SERVER.
 * @noreturn
 */
native SetGlobalTransTarget(client);
