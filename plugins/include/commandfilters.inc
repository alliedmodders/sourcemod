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
 
#if defined _commandfilters_included
 #endinput
#endif
#define _commandfilters_included

#define MAX_TARGET_LENGTH           64

#define COMMAND_FILTER_ALIVE        (1<<0)      /**< Only allow alive players */
#define COMMAND_FILTER_DEAD         (1<<1)      /**< Only filter dead players */
#define COMMAND_FILTER_CONNECTED    (1<<2)      /**< Allow players not fully in-game */
#define COMMAND_FILTER_NO_IMMUNITY  (1<<3)      /**< Ignore immunity rules */
#define COMMAND_FILTER_NO_MULTI     (1<<4)      /**< Do not allow multiple target patterns */
#define COMMAND_FILTER_NO_BOTS      (1<<5)      /**< Do not allow bots to be targetted */

#define COMMAND_TARGET_NONE          0          /**< No target was found */
#define COMMAND_TARGET_NOT_ALIVE    -1          /**< Single client is not alive */
#define COMMAND_TARGET_NOT_DEAD     -2          /**< Single client is not dead */
#define COMMAND_TARGET_NOT_IN_GAME  -3          /**< Single client is not in game */
#define COMMAND_TARGET_IMMUNE       -4          /**< Single client is immune */
#define COMMAND_TARGET_EMPTY_FILTER -5          /**< A multi-filter (such as @all) had no targets */
#define COMMAND_TARGET_NOT_HUMAN    -6          /**< Target was not human */
#define COMMAND_TARGET_AMBIGUOUS    -7          /**< Partial name had too many targets */

/**
 * Processes a generic command target string, and resolves it to a list 
 * of clients or one client, based on filtering rules and a pattern.
 *
 * Note that you should use LoadTranslations("common.phrases") in OnPluginStart(), 
 * as that file is guaranteed to contain all of the translatable phrases that 
 * ProcessTargetString() will return.
 *
 * @param pattern       Pattern to find clients against.
 * @param admin         Admin performing the action, or 0 if the server.
 * @param targets       Array to hold targets.
 * @param max_targets   Maximum size of the targets array.
 * @param filter_flags  Filter flags.
 * @param target_name   Buffer to store the target name.
 * @param tn_maxlength  Maximum length of the target name buffer.
 * @param tn_is_ml      OUTPUT: Will be true if the target name buffer is an ML phrase,
 *                      false if it is a normal string.
 * @return              If a multi-target pattern was used, the number of clients found 
 *                      is returned.  If a single-target pattern was used, 1 is returned 
 *                      if one valid client is found.  Otherwise, a COMMAND_TARGET reason 
 *                      for failure is returned.
 */
native int ProcessTargetString(const char[] pattern,
							   int admin, 
							   int[] targets,
							   int max_targets,
							   int filter_flags,
							   char[] target_name,
							   int tn_maxlength,
							   bool &tn_is_ml);

/**
 * Replies to a client with a given message describing a targetting 
 * failure reason.
 *
 * Note: The translation phrases are found in common.phrases.txt.
 *
 * @param client        Client index, or 0 for server.
 * @param reason        COMMAND_TARGET reason.
 */
stock void ReplyToTargetError(int client, int reason)
{
	switch (reason)
	{
		case COMMAND_TARGET_NONE:
		{
			ReplyToCommand(client, "[SM] %t", "No matching client");
		}
		case COMMAND_TARGET_NOT_ALIVE:
		{
			ReplyToCommand(client, "[SM] %t", "Target must be alive");
		}
		case COMMAND_TARGET_NOT_DEAD:
		{
			ReplyToCommand(client, "[SM] %t", "Target must be dead");
		}
		case COMMAND_TARGET_NOT_IN_GAME:
		{
			ReplyToCommand(client, "[SM] %t", "Target is not in game");
		}
		case COMMAND_TARGET_IMMUNE:
		{
			ReplyToCommand(client, "[SM] %t", "Unable to target");
		}
		case COMMAND_TARGET_EMPTY_FILTER:
		{
			ReplyToCommand(client, "[SM] %t", "No matching clients");
		}
		case COMMAND_TARGET_NOT_HUMAN:
		{
			ReplyToCommand(client, "[SM] %t", "Cannot target bot");
		}
		case COMMAND_TARGET_AMBIGUOUS:
		{
			ReplyToCommand(client, "[SM] %t", "More than one client matched");
		}
	}
}

#define FEATURECAP_MULTITARGETFILTER_CLIENTPARAM    "SourceMod MultiTargetFilter ClientParam"

/**
 * Adds clients to a multi-target filter.
 *
 * @param pattern       Pattern name.
 * @param clients       Array to fill with unique, valid client indexes.
 * @param client        Client that triggered this filter.
 * @return              True if pattern was recognized, false otherwise.
 *
 * @note To see if the client param is available, use FeatureType_Capability and FEATURECAP_MULTITARGETFILTER_CLIENTPARAM.
 */
typeset MultiTargetFilter {
	function bool (const char[] pattern, Handle clients);
	function bool (const char[] pattern, ArrayList clients);
	function bool (const char[] pattern, Handle clients, int client);
	function bool (const char[] pattern, ArrayList clients, int client);
}

/**
 * Adds a multi-target filter function for ProcessTargetString().
 *
 * @param pattern       Pattern to match (case sensitive).
 * @param filter        Filter function.
 * @param phrase        Descriptive phrase to display on successful match.
 * @param phraseIsML    True if phrase is multi-lingual, false otherwise.
 */
native void AddMultiTargetFilter(const char[] pattern, MultiTargetFilter filter,
                                 const char[] phrase, bool phraseIsML);

/**
 * Removes a multi-target filter function from ProcessTargetString().
 *
 * @param pattern       Pattern to match (case sensitive).
 * @param filter        Filter function.
 */
native void RemoveMultiTargetFilter(const char[] pattern, MultiTargetFilter filter);
