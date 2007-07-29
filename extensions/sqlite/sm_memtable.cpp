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

#include <string.h>
#include <malloc.h>
#include "sm_memtable.h"

BaseMemTable::BaseMemTable(unsigned int init_size)
{
	membase = (unsigned char *)malloc(init_size);
	size = init_size;
	tail = 0;
}

BaseMemTable::~BaseMemTable()
{
	free(membase);
	membase = NULL;
}

int BaseMemTable::CreateMem(unsigned int addsize, void **addr)
{
	int idx = (int)tail;

	while (tail + addsize >= size)
	{
		size *= 2;
		membase = (unsigned char *)realloc(membase, size);
	}

	tail += addsize;

	if (addr)
	{
		*addr = (void *)&membase[idx];
	}

	return idx;
}

void *BaseMemTable::GetAddress(int index)
{
	if (index < 0 || (unsigned int)index >= tail)
	{
		return NULL;
	}

	return &membase[index];
}

void BaseMemTable::Reset()
{
	tail = 0;
}

BaseStringTable::BaseStringTable(unsigned int init_size) : m_table(init_size)
{
}

BaseStringTable::~BaseStringTable()
{
}

int BaseStringTable::AddString(const char *string)
{
	size_t len = strlen(string) + 1;
	int idx;
	char *addr;

	idx = m_table.CreateMem(len, (void **)&addr);
	strcpy(addr, string);

	return idx;
}

/*const char *BaseStringTable::GetString(int str)
{
	return (const char *)m_table.GetAddress(str);
}*/

void BaseStringTable::Reset()
{
	m_table.Reset();
}
