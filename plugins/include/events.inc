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
 
#if defined _events_included
 #endinput
#endif
#define _events_included

/**
 * Event hook modes determining how hooking should be handled
 */
enum EventHookMode
{
	EventHookMode_Pre,					/**< Hook callback fired before event is fired */
	EventHookMode_Post,					/**< Hook callback fired after event is fired */
	EventHookMode_PostNoCopy			/**< Hook callback fired after event is fired, but event data won't be copied */
};

/**
 * Hook function types for events.
 */
funcenum EventHook
{
	/**
	 * Called when a game event is fired.
	 *
	 * @param event			Handle to event. This could be INVALID_HANDLE if every plugin hooking 
	 *						this event has set the hook mode EventHookMode_PostNoCopy.
	 * @param name			String containing the name of the event.
	 * @param dontBroadcast	True if event was not broadcast to clients, false otherwise.
	 * @return				Ignored for post hooks. Plugin_Handled will block event if hooked as pre.
	 */
	Action:public(Handle:event, const String:name[], bool:dontBroadcast),
	/**
	 * Called when a game event is fired.
	 *
	 * @param event			Handle to event. This could be INVALID_HANDLE if every plugin hooking 
	 *						this event has set the hook mode EventHookMode_PostNoCopy.
	 * @param name			String containing the name of the event.
	 * @param dontBroadcast	True if event was not broadcast to clients, false otherwise.
	 * @noreturn
	 */
	public(Handle:event, const String:name[], bool:dontBroadcast),
};

/**
 * Creates a hook for when a game event is fired.
 *
 * @param name			Name of event.
 * @param callback		An EventHook function pointer.
 * @param mode			Optional EventHookMode determining the type of hook.
 * @noreturn
 * @error				Invalid event name or invalid callback function.
 */
native HookEvent(const String:name[], EventHook:callback, EventHookMode:mode=EventHookMode_Post);

/**
 * Creates a hook for when a game event is fired.
 *
 * @param name			Name of event.
 * @param callback		An EventHook function pointer.
 * @param mode			Optional EventHookMode determining the type of hook.
 * @return				True if event exists and was hooked successfully, false otherwise.
 * @error				Invalid callback function.
 */
native bool:HookEventEx(const String:name[], EventHook:callback, EventHookMode:mode=EventHookMode_Post);

/**
 * Removes a hook for when a game event is fired.
 *
 * @param name			Name of event.
 * @param callback		An EventHook function pointer.
 * @param mode			Optional EventHookMode determining the type of hook.
 * @noreturn
 * @error				Invalid callback function or no active hook for specified event.
 */
native UnhookEvent(const String:name[], EventHook:callback, EventHookMode:mode=EventHookMode_Post);

/**
 * Creates a game event to be fired later.
 *
 * The Handle should not be closed via CloseHandle().  It must be closed via 
 * FireEvent() or CancelCreatedEvent().
 *
 * @param name			Name of event.
 * @param force			If set to true, this forces the event to be created even if it's not being hooked.
 *						Note that this will not force it if the event doesn't exist at all.
 * @return				Handle to event. INVALID_HANDLE is returned if the event doesn't exist or isn't 
						being hooked (unless force is true).
 */
native Handle:CreateEvent(const String:name[], bool:force=false);

/**
 * Fires a game event.
 *
 * This function closes the event Handle after completing.
 *
 * @param event			Handle to the event.
 * @param dontBroadcast	Optional boolean that determines if event should be broadcast to clients.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native FireEvent(Handle:event, bool:dontBroadcast=false);

/**
 * Cancels a previously created game event that has not been fired.
 *
 * @param event			Handled to the event.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native CancelCreatedEvent(Handle:event);

/**
 * Returns the boolean value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @return				The boolean value of the specfied event key.
 * @error				Invalid or corrupt Handle.
 */
native bool:GetEventBool(Handle:event, const String:key[]);

/**
 * Sets the boolean value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @param value			New boolean value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetEventBool(Handle:event, const String:key[], bool:value);

/**
 * Returns the integer value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @return				The integer value of the specfied event key.
 * @error				Invalid or corrupt Handle.
 */
native GetEventInt(Handle:event, const String:key[]);

/**
 * Sets the integer value of a game event's key.
 *
 * Integer value refers to anything that can be reduced to an integer.
 * The various size specifiers, such as "byte" and "short" are still 
 * integers, and only refer to how much data will actually be sent 
 * over the network (if applicable).
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @param value			New integer value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetEventInt(Handle:event, const String:key[], value);

/**
 * Returns the floating point value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @return				The floating point value of the specfied event key.
 * @error				Invalid or corrupt Handle.
 */
native Float:GetEventFloat(Handle:event, const String:key[]);

/**
 * Sets the floating point value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @param value			New floating point value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetEventFloat(Handle:event, const String:key[], Float:value);

/**
 * Retrieves the string value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @param value			Buffer to store the value of the specified event key.
 * @param maxlength		Maximum length of string buffer.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native GetEventString(Handle:event, const String:key[], String:value[], maxlength);

/**
 * Sets the string value of a game event's key.
 *
 * @param event			Handle to the event.
 * @param key			Name of event key.
 * @param value			New string value.
 * @noreturn
 * @error				Invalid or corrupt Handle.
 */
native SetEventString(Handle:event, const String:key[], const String:value[]);

/**
 * Retrieves the name of a game event.
 *
 * @param event			Handle to the event.
 * @param value			Buffer to store the name of the event.
 * @param maxlength		Maximum length of string buffer.
 * @noreturn
 * @error				Invalid or corrupt Handle.     
 */
native GetEventName(Handle:event, String:name[], maxlength);

/**
 * Sets whether an event's broadcasting will be disabled or not.
 *
 * This has no effect on events Handles that are not from HookEvent
 * or HookEventEx callbacks.
 *
 * @param event         Handle to an event from an event hook.
 * @param dontBroadcast True to disable broadcasting, false otherwise.
 * @noreturn
 * @error               Invalid Handle.
 */
native SetEventBroadcast(Handle:event, bool:dontBroadcast);

