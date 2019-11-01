/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2017 AlliedModders LLC.  All rights reserved.
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

#if defined _sdktools_entoutput_included
 #endinput
#endif
#define _sdktools_entoutput_included

/**
 * Called when an entity output is fired.
 *
 * @param output        Name of the output that fired.
 * @param caller        Entity index of the caller.
 * @param activator     Entity index of the activator.
 * @param delay         Delay in seconds? before the event gets fired.
 * @return              Anything other than Plugin_Continue will supress this event,
 *                      returning Plugin_Continue will allow it to propagate the results
 *                      of this output to any entity inputs.
 */
typeset EntityOutput
{
	function void (const char[] output, int caller, int activator, float delay);
	function Action (const char[] output, int caller, int activator, float delay);
};

/**
 * Add an entity output hook on a entity classname
 *
 * @param classname     The classname to hook.
 * @param output        The output name to hook.
 * @param callback      An EntityOutput function pointer.
 * @error               Entity Outputs disabled.
 */
native void HookEntityOutput(const char[] classname, const char[] output, EntityOutput callback);

/**
 * Remove an entity output hook.
 * @param classname     The classname to hook.
 * @param output        The output name to hook.
 * @param callback      An EntityOutput function pointer.
 * @return              True on success, false if no valid hook was found.
 * @error               Entity Outputs disabled.
 */
native bool UnhookEntityOutput(const char[] classname, const char[] output, EntityOutput callback);

/**
 * Add an entity output hook on a single entity instance
 *
 * @param entity        The entity on which to add a hook.
 * @param output        The output name to hook.
 * @param callback      An EntityOutput function pointer.
 * @param once          Only fire this hook once and then remove itself.
 * @error               Entity Outputs disabled or Invalid Entity index.
 */
native void HookSingleEntityOutput(int entity, const char[] output, EntityOutput callback, bool once=false);

/**
 * Remove a single entity output hook.
 *
 * @param entity        The entity on which to remove the hook.
 * @param output        The output name to hook.
 * @param callback      An EntityOutput function pointer.
 * @return              True on success, false if no valid hook was found.
 * @error               Entity Outputs disabled or Invalid Entity index.
 */
native bool UnhookSingleEntityOutput(int entity, const char[] output, EntityOutput callback);

/**
 * Fire a named output on an entity.
 *
 * After completion (successful or not), the current global variant is re-initialized.
 *
 * @param caller        Entity index from where the output is fired.
 * @param output        Output name.
 * @param activator     Entity index which initiated the sequence of actions (-1 for a NULL entity).
 * @param delay         Delay before firing the output.
 * @error               Invalid entity index or no mod support.
 */
native void FireEntityOutput(int caller, const char[] output, int activator=-1, float delay=0.0);
