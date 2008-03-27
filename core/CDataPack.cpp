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

#include <malloc.h>
#include <string.h>
#include "CDataPack.h"

#define DATAPACK_INITIAL_SIZE		512

CDataPack::CDataPack()
{
	m_pBase = (char *)malloc(DATAPACK_INITIAL_SIZE);
	m_capacity = DATAPACK_INITIAL_SIZE;
	Initialize();
}

CDataPack::~CDataPack()
{
	free(m_pBase);
}

void CDataPack::Initialize()
{
	m_curptr = m_pBase;
	m_size = 0;
}

void CDataPack::CheckSize(size_t typesize)
{
	if (m_curptr - m_pBase + typesize <= m_capacity)
	{
		return;
	}

	size_t pos = m_curptr - m_pBase;
	do
	{
		m_capacity *= 2;
		m_pBase = (char *)realloc(m_pBase, m_capacity);
		m_curptr = m_pBase + pos;
	} while (m_curptr - m_pBase + typesize > m_capacity);
}

void CDataPack::ResetSize()
{
	m_size = 0;
}

size_t CDataPack::CreateMemory(size_t size, void **addr)
{
	CheckSize(sizeof(size_t) + size);
	size_t pos = m_curptr - m_pBase;

	*(size_t *)m_curptr = size;
	m_curptr += sizeof(size_t);

	if (addr)
	{
		*addr = m_curptr;
	}

	m_curptr += size;
	m_size += sizeof(size_t) + size;

	return pos;
}

void CDataPack::PackCell(cell_t cell)
{
	CheckSize(sizeof(size_t) + sizeof(cell_t));

	*(size_t *)m_curptr = sizeof(cell_t);
	m_curptr += sizeof(size_t);

	*(cell_t *)m_curptr = cell;
	m_curptr += sizeof(cell_t);

	m_size += sizeof(size_t) + sizeof(cell_t);
}

void CDataPack::PackFloat(float val)
{
	CheckSize(sizeof(size_t) + sizeof(float));

	*(size_t *)m_curptr = sizeof(float);
	m_curptr += sizeof(size_t);

	*(float *)m_curptr = val;
	m_curptr += sizeof(float);

	m_size += sizeof(size_t) + sizeof(float);
}

void CDataPack::PackString(const char *string)
{
	size_t len = strlen(string);
	size_t maxsize = sizeof(size_t) + len + 1;
	CheckSize(maxsize);

	// Pack the string length first for buffer overrun checking.
	*(size_t *)m_curptr = len;
	m_curptr += sizeof(size_t);

	// Now pack the string.
	memcpy(m_curptr, string, len);
	m_curptr[len] = '\0';
	m_curptr += len + 1;

	m_size += maxsize;
}

void CDataPack::Reset() const
{
	m_curptr = m_pBase;
}

size_t CDataPack::GetPosition() const
{
	return static_cast<size_t>(m_curptr - m_pBase);
}

bool CDataPack::SetPosition(size_t pos) const
{
	if (pos > m_size-1)
	{
		return false;
	}
	m_curptr = m_pBase + pos;

	return true;
}

cell_t CDataPack::ReadCell() const
{
	if (!IsReadable(sizeof(size_t) + sizeof(cell_t)))
	{
		return 0;
	}
	if (*reinterpret_cast<size_t *>(m_curptr) != sizeof(cell_t))
	{
		return 0;
	}

	m_curptr += sizeof(size_t);

	cell_t val = *reinterpret_cast<cell_t *>(m_curptr);
	m_curptr += sizeof(cell_t);
	return val;
}

float CDataPack::ReadFloat() const
{
	if (!IsReadable(sizeof(size_t) + sizeof(float)))
	{
		return 0;
	}
	if (*reinterpret_cast<size_t *>(m_curptr) != sizeof(float))
	{
		return 0;
	}

	m_curptr += sizeof(size_t);

	float val = *reinterpret_cast<float *>(m_curptr);
	m_curptr += sizeof(float);
	return val;
}

bool CDataPack::IsReadable(size_t bytes) const
{
	return (bytes + (m_curptr - m_pBase) > m_size) ? false : true;
}

const char *CDataPack::ReadString(size_t *len) const
{
	if (!IsReadable(sizeof(size_t)))
	{
		return NULL;
	}

	size_t real_len = *(size_t *)m_curptr;

	m_curptr += sizeof(size_t);
	char *str = (char *)m_curptr;

	if ((strlen(str) != real_len) || !(IsReadable(real_len+1)))
	{
		return NULL;
	}

	if (len)
	{
		*len = real_len;
	}

	m_curptr += real_len + 1;

	return str;
}

void *CDataPack::GetMemory() const
{
	return m_curptr;
}

void *CDataPack::ReadMemory(size_t *size) const
{
	if (!IsReadable(sizeof(size_t)))
	{
		return NULL;
	}

	size_t bytecount = *(size_t *)m_curptr;
	m_curptr += sizeof(size_t);

	if (!IsReadable(bytecount))
	{
		return NULL;
	}

	void *ptr = m_curptr;

	if (size)
	{
		*size = bytecount;
	}

	m_curptr += bytecount;

	return ptr;
}
