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

#if defined _structs_included
 #endinput
#endif
#define _structs_included

/**
 * Struct Gamedata Notes:
 *
 * 	Types:					Sizes:
 *		int/int*				1/2/4
 *		float/float*			Not required
 *		char/char*				Length of char array
 *		Vector/Vector*			Not required
 *		ent/ent*				Not required
 */
 
/**
 * Retrieves an integer value from a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to retrieve.
 * @return			Integer value.
 * @error			Undefined member, or member type mismatch.
 */
native GetStructInt(Handle:strct, const String:member[]);

/**
 * Sets an integer value in a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to set.
 * @param value		Value to set.
 * @noreturn
 * @error			Undefined member, or member type mismatch.
 */
native SetStructInt(Handle:strct, const String:member[], value);

/**
 * Retrieves a float value from a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to retrieve.
 * @return			Float value.
 * @error			Undefined member, or member type mismatch.
 */
native Float:GetStructFloat(Handle:strct, const String:member[]);

/**
 * Sets a float value in a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to set.
 * @param value		Value to set.
 * @noreturn
 * @error			Undefined member, or member type mismatch.
 */
native SetStructFloat(Handle:strct, const String:member[], Float:value);

/**
 * Retrieves an integer value from a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to retrieve.
 * @param vec		Buffer for the retrieved vector.
 * @noreturn		
 * @error			Undefined member, or member type mismatch.
 */
native GetStructVector(Handle:strct, const String:member[], Float:vec[3]);

/**
 * Sets a Vector value in a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to set.
 * @param vec		Value to set.
 * @noreturn
 * @error			Undefined member, or member type mismatch.
 */
native SetStructVector(Handle:strct, const String:member[], Float:vec[3]);

/**
 * Retrieves a string from a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to retrieve.
 * @param value		Buffer for the retrieved string.
 * @param maxlen	Max length of the buffer
 * @noreturn		
 * @error			Undefined member, or member type mismatch.
 */
native GetStructString(Handle:strct, const String:member[], const String:value[], maxlen);

/**
 * Sets a string value in a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to set.
 * @param value		Value to set.
 * @noreturn
 * @error			Undefined member, or member type mismatch.
 */
native SetStructString(Handle:strct, const String:member[], String:value[]);

/**
 * Retrieves an entity handle from a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to retrieve.
 * @return			Entity index.
 * @error			Undefined member, or member type mismatch.
 */
native GetStructEnt(Handle:strct, const String:member[]);

/**
 * Sets an entity handle value in a struct.
 *
 * @param strct		Struct Handle.
 * @param member	Member name to set.
 * @param entity	Entity index to set.
 * @noreturn
 * @error			Undefined member, or member type mismatch.
 */
native SetStructEnt(Handle:strct, const String:member[], entity);

/**
 * Retrieves a struct handle to a FileWeaponInfo_t for the given weapon name.
 * This handle needs to be closed when you have finished using it.
 *
 * @param weapon	Weapon name to retrieve.
 * @return			Struct handle or INVALID_HANDLE on failure.
 */
native Handle:GetWeaponStruct(const String:weapon[]);

/**
 * Do not edit below this line!
 */
public Extension:__ext_struct = 
{
	name = "Struct Abstraction Extension",
	file = "structs.ext",
	autoload = 1,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
