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

/**
 * Retrieves the language number of a client.
 * Currently this simply returns the server language index.
 *
 * @param client	Client index.
 * @return			Language number client is using.
 * @error			Invalid client index or client not in game.
 */
native GetClientLanguage(client);

/**
 * Retrieves the server's language.
 *
 * @return			Language number server is using.
 */
native GetServerLanguage();

/**
 * Returns the number of languages known in languages.cfg.
 *
 * @return			Language count.
 */
native GetLanguageCount();

/**
 * Retrieves info about a given language number.
 *
 * @param language	Language number.
 * @param code		Language code buffer (2-3 characters usually).
 * @param codeLen	Maximum length of the language code buffer.
 * @param name		Language name buffer.
 * @param nameLen	Maximum length of the language name buffer.
 * @noreturn
 * @error			Invalid language number.
 */
native GetLanguageInfo(language, String:code[]="", codeLen=0, String:name[]="", nameLen=0);

