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

#ifndef _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_
#define _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_

#include <stdlib.h>
#include <string.h>

class BaseMemTable
{
public:
	BaseMemTable(unsigned int init_size)
	{
		membase = (unsigned char *)malloc(init_size);
		size = init_size;
		tail = 0;
	}
	~BaseMemTable()
	{
		free(membase);
		membase = NULL;
	}
public:
	/**
	 * Allocates 'size' bytes of memory.
	 * Optionally outputs the address through 'addr'.
	 * Returns an index >= 0 on success, < 0 on failure.
	 */
	int CreateMem(unsigned int addsize, void **addr)
	{
		int idx = (int)tail;

		while (tail + addsize >= size) {
			size *= 2;
			membase = (unsigned char *)realloc(membase, size);
		}

		tail += addsize;
		if (addr)
			*addr = (void *)&membase[idx];

		return idx;
	}

	/**
	 * Given an index into the memory table, returns its address.
	 * Returns NULL if invalid.
	 */
	void *GetAddress(int index)
	{
		if (index < 0 || (unsigned int)index >= tail)
			return NULL;
		return &membase[index];
	}

	/**
	 * Scraps the memory table.  For caching purposes, the memory 
	 * is not freed, however subsequent calls to CreateMem() will 
	 * begin at the first index again.
	 */
	void Reset()
	{
		tail = 0;
	}

	inline unsigned int GetMemUsage()
	{
		return size;
	}

	inline unsigned int GetActualMemUsed()
	{
		return tail;
	}

private:
	unsigned char *membase;
	unsigned int size;
	unsigned int tail;
};

class BaseStringTable
{
public:
	BaseStringTable(unsigned int init_size) : m_table(init_size)
	{
	}
public:
	/** 
	 * Adds a string to the string table and returns its index.
	 */
	int AddString(const char *string)
	{
		return AddString(string, strlen(string));
	}

	/** 
	 * Adds a string to the string table and returns its index.
	 */
	int AddString(const char *string, size_t length)
	{
		size_t len = length + 1;
		int idx;
		char *addr;

		idx = m_table.CreateMem(len, (void **)&addr);
		memcpy(addr, string, length + 1);
		return idx;
	}

	/**
	 * Given an index into the string table, returns the associated string.
	 */
	inline const char *GetString(int str)
	{
		return (const char *)m_table.GetAddress(str);
	}

	/**
	 * Scraps the string table. For caching purposes, the memory
	 * is not freed, however subsequent calls to AddString() will 
	 * begin at the first index again.
	 */
	void Reset()
	{
		m_table.Reset();
	}

	/**
	 * Returns the parent BaseMemTable that this string table uses.
	 */
	inline BaseMemTable *GetMemTable()
	{
		return &m_table;
	}
private:
	BaseMemTable m_table;
};

#endif //_INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_

