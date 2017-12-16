/**
* vim: set ts=4 :
* =============================================================================
* SourceMod
* Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
* =============================================================================
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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_KEYVALUESTACK_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_KEYVALUESTACK_H_

/**
* @file IKeyValueStack.h
* @brief Interface to the data structure that SP uses to interact with KeyValues. 
* The wrappers for creating these are contained in ISourceMod.h
*/

/**
 * @brief Forward declaration of the KeyValues class.
 */
class KeyValues;

namespace SourceMod
{
	/**
	 * @brief Defines a KeyValues Stack.
	 */
	class IKeyValueStack
	{
	public:
		/**
		 * @brief Defines possible results that can be returned by ``DeleteThis``.
		 */
		enum DeleteThis_Result {
			DeleteThis_Failed = 0,		/**< Failed to delete the current section */
			DeleteThis_MovedNext = 1,	/**< Success, moved to next section */
			DeleteThis_MovedBack = -1,	/**< Success, moved one section back */
		};

		/**
		 * Retrieves the root section.
		 * 
		 * @return	The root section.
		 */
		virtual KeyValues *GetRootSection() = 0;

		/**
		 * Retrieves the current section, the last node on the path stack. 
		 * 
		 * @return	The current section.
		 */
		virtual KeyValues *GetCurrentSection() = 0;

		/**
		 * Sets the current position in the KeyValues tree to the given key.
		 * 
		 * @param keyName	Name of the key.
		 * @param create	If true, and the key does not exist, it will be created.
		 * @return			True if the key exists, false if it does not and was not created.
		 */
		virtual bool JumpToKey(const char* keyName, bool create = false) = 0;

		/**
		 * Sets the current position in the KeyValues tree to the given key.
		 * 
		 * @param symbol	Name of the key.
		 * @return			True if the key exists, false if it does not.
		 */
		virtual bool JumpToKeySymbol(int symbol) = 0;
		
		/**
		 * Sets the current position in the tree to the next sub key.
		 * 
		 * @param keyOnly	If false, non-keys will be traversed (values).
		 * @return			True on success, false if there was no next sub key.
		 */
		virtual bool GotoNextKey(bool keyOnly) = 0;

		/**
		 * Sets the current position in the KeyValues tree to the first sub key.
		 * 
		 * @param keyOnly	If false, non-keys will be traversed (values).
		 * @return			True on success, false if there was no first sub key.
		 */
		virtual bool GotoFirstSubKey(bool keyOnly) = 0;

		/**
		 * Saves the current position in the traversal stack onto the traversal
		 * stack. This can be useful if you wish to use ``GotoNextKey`` and
		 * have the previous key saved for backwards traversal.
		 * 
		 * @return			False if the current section is the root section.
		 */
		virtual bool SavePosition() = 0;

		/**
		 * Jumps back to the previous position. Returns false if there are no
		 * previous positions (i.e., at the root node). This should be called
		 * once for each successful Jump call, in order to return to the top node.
		 * This function pops one node off the internal traversal stack.
		 * 
		 * @return			True on success, false if there is no higher node.
		 */
		virtual bool GoBack() = 0;

		/**
		 * Removes the current sub-key and attempts to set the position
		 * to the sub-key after the removed one. If no such sub-key exists,
		 * the position will be the parent key in the traversal stack.
		 * Given the sub-key having position "N" in the traversal stack, the
		 * removal will always take place from position "N-1."
		 * 
		 * @return			A ``DeleteThis_Result`` value stating the result.
		 */
		virtual DeleteThis_Result DeleteThis() = 0;

		/**
		 * Sets the position back to the top node, emptying the entire node
		 * traversal history. This can be used instead of looping ``GoBack``
		 * if recursive iteration is not important.
		 */
		virtual void Rewind() = 0;

		/**
		 * Returns the position in the jump stack; I.e. the number of calls
		 * required for KvGoBack to return to the root node. If at the root node,
		 * 0 is returned.
		 * 
		 * @return			Number of non-root nodes in the jump stack.
		 */
		virtual unsigned int GetNodeCount() = 0;

		/**
		 * @brief Returns a computed aproximate size of this object, 
		 * including the KV object.
		 * 
		 * @return			Aproximate size of this structure and the KV object.
		 */
		virtual unsigned int CalcSize() = 0;

		/**
		 * @brief Returns True if the KeyValues object should be deleted along with
		 * this object. 
		 * 
		 * @return			True if the KV object should be deleted when this gets deleted.
		 */
		virtual bool DeleteKVOnDestroy() = 0;

		/**
		 * @brief Sets whether or not the KeyValues root must be deleted when
		 * this object gets deleted.
		 * 
		 * @param deleteOnDestroy	
		 */
		virtual void SetDeleteKVOnDestroy(bool deleteOnDestroy) = 0;
	};
}

#endif