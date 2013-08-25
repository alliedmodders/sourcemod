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

#ifndef _INCLUDE_SOURCEMOD_ADT_FACTORY_H_
#define _INCLUDE_SOURCEMOD_ADT_FACTORY_H_

#include <IShareSys.h>

#define SMINTERFACE_ADTFACTORY_NAME		"IADTFactory"
#define SMINTERFACE_ADTFACTORY_VERSION	2

/**
* @file IADTFactory.h
* @brief Creates abstract data types.
*/

namespace SourceMod
{
	/**
	 * @brief A hash table data type.
	 */
	class IBasicTrie
	{
	public:
		/**
		 * @brief Inserts a key/value pair.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Value pointer (may be anything).
		 * @return			True on success, false if key already exists.
		 */
		virtual bool Insert(const char *key, void *value) =0;

		/**
		 * @brief Retrieves the value of a key.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Optional pointer to store value pointer.
		 * @return			True on success, false if key was not found.
		 */
		virtual bool Retrieve(const char *key, void **value) =0;

		/**
		 * @brief Deletes a key.
		 *
		 * @param key		Key string (null terminated).
		 * @return			True on success, false if key was not found.
		 */
		virtual bool Delete(const char *key) =0;

		/**
		 * @brief Flushes the entire trie of all keys.
		 */
		virtual void Clear() =0;

		/**
		 * @brief Destroys the IBasicTrie object and frees all associated 
		 * memory.
		 */
		virtual void Destroy() =0;

		/**
		 * @brief Inserts a key/value pair, replacing an old inserted
		 * value if it already exists.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Value pointer (may be anything).
		 * @return			True on success, false on failure.
		 */
		virtual bool Replace(const char *key, void *value) =0;
	};

	class IADTFactory : public SMInterface
	{
	public:
		/**
		 * @brief Creates a basic Trie object.
		 *
		 * @return			A new IBasicTrie object which must be destroyed
		 *					via IBasicTrie::Destroy().
		 */
		virtual IBasicTrie *CreateBasicTrie() =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_ADT_FACTORY_H_
