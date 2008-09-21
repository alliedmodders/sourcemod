/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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
 * @param output		Name of the output that fired.
 * @param caller		Entity index of the caller.
 * @param activator		Entity index of the activator.
 * @param delay			Delay in seconds? before the event gets fired.
 */
functag public EntityOutput(const String:output[], caller, activator, Float:delay);

/**
 * Add an entity output hook on a entity classname
 *
 * @param classname		The classname to hook.
 * @param output		The output name to hook.
 * @param callback		An EntityOutput function pointer.
 * @noreturn
 * @error				Entity Outputs disabled.
 */
native HookEntityOutput(const String:classname[], const String:output[], EntityOutput:callback);

/**
 * Remove an entity output hook.
 * @param classname		The classname to hook.
 * @param output		The output name to hook.
 * @param callback		An EntityOutput function pointer.
 * @return				True on success, false if no valid hook was found.
 * @error				Entity Outputs disabled.
 */
native bool:UnhookEntityOutput(const String:classname[], const String:output[], EntityOutput:callback);

/**
 * Add an entity output hook on a single entity instance	
 *
 * @param entity		The entity on which to add a hook.
 * @param output		The output name to hook.
 * @param callback		An EntityOutput function pointer.
 * @param once			Only fire this hook once and then remove itself.
 * @noreturn			
 * @error				Entity Outputs disabled	or Invalid Entity index.
 */
native HookSingleEntityOutput(entity, const String:output[], EntityOutput:callback , bool:once=false);

/**
 * Remove a single entity output hook.	
 *
 * @param entity		The entity on which to remove the hook.
 * @param output		The output name to hook.
 * @param callback		An EntityOutput function pointer.
 * @return				True on success, false if no valid hook was found.
 * @error				Entity Outputs disabled	or Invalid Entity index.	
 */
native bool:UnhookSingleEntityOutput(entity, const String:output[], EntityOutput:callback);

