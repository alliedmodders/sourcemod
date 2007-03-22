/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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
