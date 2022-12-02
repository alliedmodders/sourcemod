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

#if defined _sdktools_engine_included
 #endinput
#endif
#define _sdktools_engine_included

#define MAX_LIGHTSTYLES 64

/**
 * Sets a client's "viewing entity."
 *
 * @param client        Client index.
 * @param entity        Entity index.
 * @error               Invalid client or entity, lack of mod support, or client not in 
 *                      game.
 */
native void SetClientViewEntity(int client, int entity);

/**
 * Sets a light style.
 *
 * @param style         Light style (from 0 to MAX_LIGHTSTYLES-1)
 * @param value         Light value string (see world.cpp/light.cpp in dlls)
 * @error               Light style index is out of range.
 */
native void SetLightStyle(int style, const char[] value);

/**
 * Returns the client's eye position.
 *
 * @param client        Player's index.
 * @param pos           Destination vector to store the client's eye position.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void GetClientEyePosition(int client, float pos[3]);
