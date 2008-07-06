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

#ifndef _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_
#define _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_

class BaseMemTable
{
public:
	BaseMemTable(unsigned int init_size);
	~BaseMemTable();
public:
	/**
	 * Allocates 'size' bytes of memory.
	 * Optionally outputs the address through 'addr'.
	 * Returns an index >= 0 on success, < 0 on failure.
	 */
	int CreateMem(unsigned int size, void **addr);

	/**
	 * Given an index into the memory table, returns its address.
	 * Returns NULL if invalid.
	 */
	void *GetAddress(int index);

	/**
	 * Scraps the memory table.  For caching purposes, the memory 
	 * is not freed, however subsequent calls to CreateMem() will 
	 * begin at the first index again.
	 */
	void Reset();

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
	BaseStringTable(unsigned int init_size);
	~BaseStringTable();
public:
	/** 
	 * Adds a string to the string table and returns its index.
	 */
	int AddString(const char *string);

	/**
	 * Given an index into the string table, returns the associated string.
	 */
	inline const char *GetString(int str)
	{
		return (const char *)m_table.GetAddress(str);
	}

	/**
	 * Scraps the string table.  For caching purposes, the memory
	 * is not freed, however subsequent calls to AddString() will 
	 * begin at the first index again.
	 */
	void Reset();

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
