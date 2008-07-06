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

#if defined _handles_included
 #endinput
#endif
#define _handles_included

/**
 * Handle helper macros.
 */
enum Handle
{
	INVALID_HANDLE = 0,
};


/**
 * Closes a Handle.  If the handle has multiple copies open, 
 * it is not destroyed unless all copies are closed.
 * 
 * @note Closing a Handle has a different meaning for each Handle type.  Make
 *       sure you read the documentation on whatever provided the Handle.
 *
 * @param hndl		Handle to close.
 * @return			True if successful, false if not closeable.
 * @error			Invalid handles will cause a run time error.
 */
native bool:CloseHandle(Handle:hndl);

/**
 * Clones a Handle.  When passing handles in between plugins, caching handles
 * can result in accidental invalidation when one plugin releases the Handle, or is its owner
 * is unloaded from memory.  To prevent this, the Handle may be "cloned" with a new owner.
 * 
 * @note Usually, you will be cloning Handles for other plugins.  This means that if you clone
 * the Handle without specifying the new owner, it will assume the identity of your original calling
 * plugin, which is not very useful.  You should either specify that the receiving plugin should
 * clone the handle on its own, or you should explicitly clone the Handle using the receiving plugin's
 * identity Handle.
 *
 * @param hndl		Handle to clone/duplicate.
 * @param plugin	Optional Handle to another plugin to mark as the new owner.
 *					If no owner is passed, the owner becomes the calling plugin.
 * @return			Handle on success, INVALID_HANDLE if not cloneable.
 * @error			Invalid handles will cause a run time error.
 */
native Handle:CloneHandle(Handle:hndl, Handle:plugin=INVALID_HANDLE);

/**
 * Do not use this function.  Returns if a Handle and its contents 
 * are readable, whereas INVALID_HANDLE only checks for the absence 
 * of a Handle.
 *
 * This function is intended only for tests where the validity of a 
 * Handle can absolutely not be known. 
 *
 * Do not use this to check the return values of functions, or to 
 * check if timers should be closed (except in very rare cases).  
 * This function is for very specific usage and using it for general 
 * purpose routines can and will hide very subtle bugs.
 *
 * @param hndl		Handle to test for validity.
 * @return			True if handle is valid, false otherwise.
 */
#pragma deprecated Do not use this function.
native bool:IsValidHandle(Handle:hndl);
