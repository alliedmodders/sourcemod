/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SQLite Driver Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
