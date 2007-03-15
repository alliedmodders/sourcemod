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
 * @brief Returns if a handle is valid or not.
 * @note It is not a good idea to call this on every Handle.  If you code properly,
 *       all of your Handles will either be valid or will expose important bugs to fix.
 *       This is provided for situations only where testing for handle validity is needed.
 *
 * @param hndl		Handle to test for validity.
 * @return			True if handle is valid, false otherwise.
 */
native bool:IsValidHandle(Handle:hndl);

/**
 * @brief Closes a Handle.  If the handle has multiple copies open, 
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
 * @brief Clones a Handle.  When passing handles in between plugins, caching handles
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
