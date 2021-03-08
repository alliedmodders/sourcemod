/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#ifndef _include_sourcemod_namehashset_h_
#define _include_sourcemod_namehashset_h_

/**
 * @file sm_namehashset.h
 *
 * @brief Stores a set of uniquely named objects.
 */

#include <am-allocator-policies.h>
#include <am-hashmap.h>
#include <am-string.h>
#include "sm_stringhashmap.h"

namespace SourceMod
{

// The HashPolicy type must have these methods:
// 		static bool matches(const char *key, const T &value);
// 		static uint32_t hash(const CharsAndLength &key);
//
// Depending on what lookup types are used, and how hashing should be done.
// Most of the time, key hashing will just call the key's hash() method.
//
// If these members are available on T, then the HashPolicy type can be left
// default. It is okay to use |T *|, the functions will still be looked up
// on |T|.
template <typename T, typename KeyPolicy = T>
class NameHashSet : public ke::SystemAllocatorPolicy
{
	typedef detail::CharsAndLength CharsAndLength;

	// Default policy type: the two types are different. Use them directly.
	template <typename KeyType, typename KeyPolicyType>
	struct Policy
	{
		typedef KeyType Payload;

		static uint32_t hash(const CharsAndLength &key)
		{
			return KeyPolicyType::hash(key);
		}

		static bool matches(const CharsAndLength &key, const KeyType &value)
		{
			return KeyPolicyType::matches(key.c_str(), value);
		}
	};

	// Specialization: the types are equal, and T is a pointer. Strip the
	// pointer off so we can access T:: for match functions.
	template <typename KeyType>
	struct Policy<KeyType *, KeyType *>
	{
		typedef KeyType *Payload;

		static uint32_t hash(const CharsAndLength &key)
		{
			return KeyType::hash(key);
		}

		static bool matches(const CharsAndLength &key, const KeyType *value)
		{
			return KeyType::matches(key.c_str(), value);
		}
	};

	typedef ke::HashTable<Policy<T, KeyPolicy>, ke::SystemAllocatorPolicy> Internal;

public:
	NameHashSet()
	{
		if (!table_.init())
			this->reportOutOfMemory();
	}

	typedef typename Internal::Result Result;
	typedef typename Internal::Insert Insert;
	typedef typename Internal::iterator iterator;

	Result find(const char *aKey)
	{
		return table_.find(aKey);
	}

	Insert findForAdd(const char *aKey)
	{
		return table_.findForAdd(aKey);
	}

	template <typename U>
	bool add(Insert &i, U &&value)
	{
		return table_.add(i, std::forward<U>(value));
	}

	bool retrieve(const char *aKey, T *value)
	{
		CharsAndLength key(aKey);
		Result r = table_.find(aKey);
		if (!r.found())
			return false;
		*value = *r;
		return true;
	}

	template <typename U>
	bool insert(const char *aKey, U &&value)
	{
		CharsAndLength key(aKey);
		Insert i = table_.findForAdd(key);
		if (i.found())
			return false;
		return table_.add(i, std::forward<U>(value));
	}

	bool contains(const char *aKey)
	{
		CharsAndLength key(aKey);
		Result r = table_.find(aKey);
		return r.found();
	}

	bool remove(const char *aKey)
	{
		CharsAndLength key(aKey);
		Result r = table_.find(key);
		if (!r.found())
			return false;
		table_.remove(r);
		return true;
	}

	void remove(Result &r)
	{
		table_.remove(r);
	}

	void clear()
	{
		table_.clear();
	}

	size_t mem_usage() const
	{
		return table_.estimateMemoryUse();
	}

	iterator iter()
	{
		return iterator(&table_);
	}

private:
	Internal table_;
};

}

#endif // _include_sourcemod_namehashset_h_
