/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
 */

#ifndef _INCLUDE_SOURCEMOD_CORE_SYMBOLTABLE_H_
#define _INCLUDE_SOURCEMOD_CORE_SYMBOLTABLE_H_

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define KESTRING_TABLE_START_SIZE 65536

struct Symbol
{
	size_t length;
	uint32_t hash;
	void *address;
	Symbol *tbl_next;

	inline char *buffer()
	{
		return reinterpret_cast<char *>(this + 1);
	}
};

class SymbolTable
{
public:
	~SymbolTable()
	{
		for (uint32_t i = 0; i < nbuckets; i++)
		{
			Symbol *sym = buckets[i];
			while (sym != NULL)
			{
				Symbol *next = sym->tbl_next;
				free(sym);
				sym = next;
			}
		}
		free(buckets);
	}

	bool Initialize()
	{
		buckets = (Symbol **)malloc(sizeof(Symbol *) * KESTRING_TABLE_START_SIZE);
		if (buckets == NULL)
		{
			return false;
		}
		memset(buckets, 0, sizeof(Symbol *) * KESTRING_TABLE_START_SIZE);

		nbuckets = KESTRING_TABLE_START_SIZE;
		nused = 0;
		bucketmask = KESTRING_TABLE_START_SIZE - 1;
		return true;
	}

	static inline uint32_t HashString(const char *data, size_t len)
	{
		#undef get16bits
		#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
				|| defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
			#define get16bits(d) (*((const uint16_t *) (d)))
		#endif
		#if !defined (get16bits)
			#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
											 +(uint32_t)(((const uint8_t *)(d))[0]) )
		#endif
		uint32_t hash = len, tmp;
		int rem;

		if (len <= 0 || data == NULL)
		{
			return 0;
		}

		rem = len & 3;
		len >>= 2;

		/* Main loop */
		for (;len > 0; len--) {
				hash	+= get16bits (data);
				tmp		= (get16bits (data+2) << 11) ^ hash;
				hash	 = (hash << 16) ^ tmp;
				data	+= 2 * sizeof (uint16_t);
				hash	+= hash >> 11;
		}

		/* Handle end cases */
		switch (rem) {
				case 3: hash += get16bits (data);
								hash ^= hash << 16;
								hash ^= data[sizeof (uint16_t)] << 18;
								hash += hash >> 11;
								break;
				case 2: hash += get16bits (data);
								hash ^= hash << 11;
								hash += hash >> 17;
								break;
				case 1: hash += *data;
								hash ^= hash << 10;
								hash += hash >> 1;
		}

		/* Force "avalanching" of final 127 bits */
		hash ^= hash << 3;
		hash += hash >> 5;
		hash ^= hash << 4;
		hash += hash >> 17;
		hash ^= hash << 25;
		hash += hash >> 6;

		return hash;

		#undef get16bits
	}

	Symbol **FindSymbolBucket(const char *str, size_t len, uint32_t hash)
	{
		uint32_t bucket = hash & bucketmask;
		Symbol **pkvs = &buckets[bucket];

		Symbol *kvs = *pkvs;
		while (kvs != NULL)
		{
			if (len == kvs->length && memcmp(str, kvs->buffer(), len * sizeof(char)) == 0)
			{
				return pkvs;
			}
			pkvs = &kvs->tbl_next;
			kvs = *pkvs;
		}

		return pkvs;
	}

	void ResizeSymbolTable()
	{
		uint32_t xnbuckets = nbuckets * 2;
		Symbol **xbuckets = (Symbol **)malloc(sizeof(Symbol *) * xnbuckets);
		if (xbuckets == NULL)
		{
			return;
		}
		memset(xbuckets, 0, sizeof(Symbol *) * xnbuckets);
		uint32_t xbucketmask = xnbuckets - 1;
		for (uint32_t i = 0; i < nbuckets; i++)
		{
			Symbol *sym = buckets[i];
			while (sym != NULL)
			{
				Symbol *next = sym->tbl_next;
				uint32_t bucket = sym->hash & xbucketmask;
				sym->tbl_next = xbuckets[bucket];
				xbuckets[bucket] = sym;
				sym = next;
			}
		}
		free(buckets);
		buckets = xbuckets;
		nbuckets = xnbuckets;
		bucketmask = xbucketmask;
	}

	Symbol *FindSymbol(const char *str, size_t len)
	{
		uint32_t hash = HashString(str, len);
		Symbol **pkvs = FindSymbolBucket(str, len, hash);
		return *pkvs;
	}

	Symbol *InternSymbol(const char* str, size_t len, void *address)
	{
		uint32_t hash = HashString(str, len);
		Symbol **pkvs = FindSymbolBucket(str, len, hash);
		if (*pkvs != NULL)
		{
			return *pkvs;
		}

		Symbol *kvs = (Symbol *)malloc(sizeof(Symbol) + sizeof(char) * (len + 1));
		kvs->length = len;
		kvs->hash = hash;
		kvs->address = address;
		kvs->tbl_next = NULL;
		memcpy(kvs + 1, str, sizeof(char) * (len + 1));
		*pkvs = kvs;
		nused++;

		if (nused > nbuckets && nbuckets <= INT_MAX / 2)
		{
			ResizeSymbolTable();
		}

		return kvs;
	}
private:
	uint32_t nbuckets;
	uint32_t nused;
	uint32_t bucketmask;
	Symbol **buckets;
};

class AddrTable
{
public:
	~AddrTable()
	{
		for (uint32_t i = 0; i < nbuckets; i++)
		{
			Symbol *sym = buckets[i];
			while (sym != NULL)
			{
				Symbol *next = sym->tbl_next;
				free(sym);
				sym = next;
			}
		}
		free(buckets);
	}

	bool Initialize()
	{
		buckets = (Symbol **)malloc(sizeof(Symbol *) * KESTRING_TABLE_START_SIZE);
		if (buckets == NULL)
		{
			return false;
		}
		memset(buckets, 0, sizeof(Symbol *) * KESTRING_TABLE_START_SIZE);

		nbuckets = KESTRING_TABLE_START_SIZE;
		nused = 0;
		bucketmask = KESTRING_TABLE_START_SIZE - 1;
		return true;
	}

	//Knuth's quick 32-bit integer hash, might not be appropriate but I don't think I care much!
	static inline uint32_t HashAddr(void *addr)
	{
		return (uint32_t)addr * (uint32_t)2654435761u;
	}

	Symbol **FindSymbolBucket(void *addr, uint32_t hash)
	{
		uint32_t bucket = hash & bucketmask;
		Symbol **pkvs = &buckets[bucket];

		Symbol *kvs = *pkvs;
		while (kvs != NULL)
		{
			if (kvs->address == addr)
			{
				return pkvs;
			}
			pkvs = &kvs->tbl_next;
			kvs = *pkvs;
		}

		return pkvs;
	}

	void ResizeSymbolTable()
	{
		uint32_t xnbuckets = nbuckets * 2;
		Symbol **xbuckets = (Symbol **)malloc(sizeof(Symbol *) * xnbuckets);
		if (xbuckets == NULL)
		{
			return;
		}
		memset(xbuckets, 0, sizeof(Symbol *) * xnbuckets);
		uint32_t xbucketmask = xnbuckets - 1;
		for (uint32_t i = 0; i < nbuckets; i++)
		{
			Symbol *sym = buckets[i];
			while (sym != NULL)
			{
				Symbol *next = sym->tbl_next;
				uint32_t bucket = sym->hash & xbucketmask;
				sym->tbl_next = xbuckets[bucket];
				xbuckets[bucket] = sym;
				sym = next;
			}
		}
		free(buckets);
		buckets = xbuckets;
		nbuckets = xnbuckets;
		bucketmask = xbucketmask;
	}

	Symbol *FindAddr(void *addr)
	{
		uint32_t hash = HashAddr(addr);
		Symbol **pkvs = FindSymbolBucket(addr, hash);
		return *pkvs;
	}

	Symbol *InternSymbol(const char* str, size_t len, void *address)
	{
		uint32_t hash = HashAddr(address);
		Symbol **pkvs = FindSymbolBucket(address, hash);
		if (*pkvs != NULL)
		{
			return *pkvs;
		}

		Symbol *kvs = (Symbol *)malloc(sizeof(Symbol) + sizeof(char) * (len + 1));
		kvs->length = len;
		kvs->hash = hash;
		kvs->address = address;
		kvs->tbl_next = NULL;
		memcpy(kvs + 1, str, sizeof(char) * (len + 1));
		*pkvs = kvs;
		nused++;

		if (nused > nbuckets && nbuckets <= INT_MAX / 2)
		{
			ResizeSymbolTable();
		}

		return kvs;
	}
private:
	uint32_t nbuckets;
	uint32_t nused;
	uint32_t bucketmask;
	Symbol **buckets;
};

#endif //_INCLUDE_SOURCEMOD_CORE_SYMBOLTABLE_H_
