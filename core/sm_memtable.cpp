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

#include <string.h>
#include <stdlib.h>
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
